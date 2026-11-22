/*
 * SPK vision: image encoding through the multimodal projector, plus the
 * one-shot caption / VQA entry point used by the app's camera button.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <cstring>

bool spk_supports_vision(const spk_engine * eng) {
    return eng != nullptr && !eng->vision.path.empty();
}

spk_status spk_vision_eval(spk_engine * eng, spk_session * ses,
                           const std::string & prompt,
                           const std::vector<std::string> & images,
                           spk_pos n_past, bool logits_last, spk_pos * new_n_past) {
    if (eng == nullptr || ses == nullptr || new_n_past == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    if (images.empty()) {
        return SPK_ERR_INVALID_ARGUMENT;
    }

    {
        std::lock_guard<std::mutex> lock(eng->mu);
        const spk_status st = spk_slot_load(eng, eng->vision);
        if (st != SPK_OK) {
            return st;
        }
    }
    if (eng->vision.vision == nullptr) {
        return SPK_ERR_VISION_UNAVAILABLE;
    }

    spk_vision_opt opt = spk_vision_opt_default();

    std::vector<spk_bitmap *> bitmaps;
    bitmaps.reserve(images.size());
    for (const auto & path : images) {
        spk_bitmap_wrapper w = spk_vision_bitmap_from_file(eng->vision.vision,
                                                           path.c_str(), false, opt);
        if (w.bitmap == nullptr) {
            for (spk_bitmap * b : bitmaps) {
                spk_vision_bitmap_free(b);
            }
            spk_set_error(eng, "cannot decode image %s", path.c_str());
            return SPK_ERR_INVALID_ARGUMENT;
        }
        bitmaps.push_back(w.bitmap);
    }

    spk_input_chunks * chunks = spk_vision_chunks_init();
    spk_input_text text{};
    text.text          = prompt.c_str();
    text.text_len      = prompt.size();
    text.add_special   = true;
    text.parse_special = true;

    const int32_t rc_tok = spk_vision_tokenize(eng->vision.vision, chunks, &text,
                                               bitmaps.data(), bitmaps.size());
    if (rc_tok != 0) {
        spk_set_error(eng, "mtmd tokenize failed (%d)", rc_tok);
        spk_vision_chunks_free(chunks);
        for (spk_bitmap * b : bitmaps) {
            spk_vision_bitmap_free(b);
        }
        return SPK_ERR_INTERNAL;
    }

    const int32_t nb = spk_effective_batch(eng);
    const int32_t rc_eval = spk_vision_helper_eval(eng->vision.vision, eng->text.ctx, chunks,
                                                 n_past, ses->seq, nb, logits_last, new_n_past);

    spk_vision_chunks_free(chunks);
    for (spk_bitmap * b : bitmaps) {
        spk_vision_bitmap_free(b);
    }

    if (rc_eval != 0) {
        spk_set_error(eng, "vision eval failed (%d)", rc_eval);
        return SPK_ERR_CONTEXT;
    }
    ses->n_past = *new_n_past;
    return SPK_OK;
}

spk_status spk_describe_image(spk_engine * eng, const char * image_path,
                              const char * question, const spk_gen_params * params,
                              spk_token_cb on_token, void * user_data) {
    if (eng == nullptr || image_path == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    if (eng->vision.path.empty()) {
        return SPK_ERR_VISION_UNAVAILABLE;
    }

    spk_session * ses = nullptr;
    spk_status st = spk_session_create(eng, &ses);
    if (st != SPK_OK) {
        return st;
    }
    ses->system = "You are LLAI. Look at the image and answer briefly and accurately, "
                  "in the language the question was asked in.";

    const char * const imgs[2] = { image_path, nullptr };
    st = spk_session_append(ses, "user",
                            question != nullptr && question[0] != '\0'
                                ? question : "Describe this image in one or two sentences.",
                            imgs);
    if (st == SPK_OK) {
        st = spk_session_generate(ses, params, on_token, user_data);
    }
    spk_session_destroy(ses);
    return st;
}
