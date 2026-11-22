/*
 * SPK translation: a dedicated translation SLM when one is attached, otherwise
 * the primary SLM is prompted. Encoder-decoder models (T5 family) go through
 * the encoder path; decoder-only models get a strict instruction prompt.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <cstring>
#include <vector>

namespace {

void emit_piece(spk_token_cb cb, void * ud, const spk_vocab * vocab,
                spk_token_id id, bool final) {
    if (cb == nullptr) {
        return;
    }
    if (final) {
        spk_emit(cb, ud, nullptr, true);
        return;
    }
    char buf[256];
    const int32_t n = spk_token_to_piece(vocab, id, buf, (int32_t) sizeof(buf) - 1, 0, false);
    if (n > 0) {
        buf[n] = '\0';
        spk_emit(cb, ud, buf, false);
    }
}

// shared decode loop, works on any context
int spk_decode_loop(spk_engine * eng, spk_context * ctx, spk_model * model,
                    spk_sampler * smpl, spk_pos * io_past,
                    int32_t max_tokens, spk_token_cb cb, void * ud) {
    const spk_vocab * vocab = spk_model_get_vocab(model);
    const int32_t n_ctx = (int32_t) spk_n_ctx(ctx);

    for (int32_t i = 0; i < max_tokens; ++i) {
        if (eng->cancel.load()) {
            return -1;
        }
        if (*io_past + 1 >= n_ctx) {
            return -1;
        }

        const spk_token_id id = spk_sampler_sample(smpl, ctx, -1);
        if (spk_vocab_is_eog(vocab, id)) {
            break;
        }
        emit_piece(cb, ud, vocab, id, false);
        spk_sampler_accept(smpl, id);

        spk_batch one = spk_batch_init(1, 0, 1);
        one.n_tokens = 1;
        one.token[0]     = id;
        one.pos[0]       = *io_past;
        one.n_seq_id[0]  = 1;
        one.seq_id[0][0] = 0;
        one.logits[0]    = 1;
        const int32_t rc = spk_decode(ctx, one);
        spk_batch_free(one);
        if (rc != 0) {
            return -1;
        }
        *io_past += 1;
    }
    return 0;
}

} // namespace

bool spk_supports_translate(const spk_engine * eng) {
    return eng != nullptr && !eng->translate.path.empty();
}

spk_status spk_translate(spk_engine * eng, const char * text,
                         const char * src_lang, const char * tgt_lang,
                         const spk_gen_params * params,
                         spk_token_cb on_token, void * user_data) {
    if (eng == nullptr || text == nullptr || text[0] == '\0') {
        return SPK_ERR_INVALID_ARGUMENT;
    }

    const std::string target = tgt_lang != nullptr ? tgt_lang : "Chinese";
    const std::string source = src_lang != nullptr ? src_lang : "the source language";

    // pick the model: dedicated translation slot if it exists, else the SLM
    std::string path;
    {
        std::lock_guard<std::mutex> lock(eng->mu);
        if (!eng->translate.path.empty()) {
            const spk_status st = spk_slot_load(eng, eng->translate);
            if (st != SPK_OK) {
                return st;
            }
        }
    }

    spk_context * ctx   = nullptr;
    spk_model *   model = nullptr;
    bool encoder_decoder = false;

    if (eng->translate.resident) {
        ctx   = eng->translate.ctx;
        model = eng->translate.model;
        encoder_decoder = spk_model_has_encoder(model) && spk_model_has_decoder(model);
    } else {
        ctx   = eng->text.ctx;
        model = eng->text.model;
    }
    if (ctx == nullptr || model == nullptr) {
        return SPK_ERR_CONTEXT;
    }

    eng->cancel.store(false);

    const spk_vocab * vocab = spk_model_get_vocab(model);
    spk_sampler * smpl = spk_make_sampler(params, spk_vocab_n_tokens(vocab),
                                          spk_engine_arch(eng).c_str());
    if (smpl == nullptr) {
        return SPK_ERR_INTERNAL;
    }

    spk_pos past = 0;
    spk_status status = SPK_OK;
    spk_memory_clear(spk_get_memory(ctx), false);

    if (encoder_decoder) {
        // ---- encoder path (T5 family, m2m100 style) -------------------
        std::string instruction = "translate " + source + " to " + target + ": " + text;
        std::vector<spk_token_id> toks(instruction.size() + 32);
        int32_t n = spk_tokenize(vocab, instruction.c_str(), (int32_t) instruction.size(),
                                 toks.data(), (int32_t) toks.size(), true, false);
        if (n <= 0) {
            spk_set_error(eng, "encoder tokenization failed");
            spk_sampler_free(smpl);
            return SPK_ERR_INTERNAL;
        }
        toks.resize((size_t) n);

        spk_batch enc = spk_batch_init((int32_t) toks.size(), 0, 1);
        enc.n_tokens = (int32_t) toks.size();
        for (int32_t i = 0; i < (int32_t) toks.size(); ++i) {
            enc.token[i]      = toks[(size_t) i];
            enc.pos[i]        = i;
            enc.n_seq_id[i]   = 1;
            enc.seq_id[i][0]  = 0;
            enc.logits[i]     = 0;
        }
        const int32_t rc = spk_encode(ctx, enc);
        spk_batch_free(enc);
        if (rc != 0) {
            spk_set_error(eng, "encoder pass failed (%d)", rc);
            spk_sampler_free(smpl);
            return SPK_ERR_CONTEXT;
        }

        const spk_token_id start = (spk_token_id) spk_model_decoder_start_token(model);
        spk_batch one = spk_batch_init(1, 0, 1);
        one.n_tokens = 1;
        one.token[0]     = start;
        one.pos[0]       = 0;
        one.n_seq_id[0]  = 1;
        one.seq_id[0][0] = 0;
        one.logits[0]    = 1;
        const int32_t rc2 = spk_decode(ctx, one);
        spk_batch_free(one);
        if (rc2 != 0) {
            spk_set_error(eng, "decoder start failed (%d)", rc2);
            spk_sampler_free(smpl);
            return SPK_ERR_CONTEXT;
        }
        past = 1;

        const int32_t max_tokens = (params != nullptr && params->max_tokens > 0)
            ? params->max_tokens : 256;
        if (spk_decode_loop(eng, ctx, model, smpl, &past, max_tokens, on_token, user_data) != 0) {
            status = eng->cancel.load() ? SPK_ERR_CANCELLED : SPK_ERR_CONTEXT;
        }
    } else {
        // ---- instruction path ------------------------------------------
        std::string prompt =
            "<|im_start|>system\nYou are a translation engine. Output only the translation, "
            "no explanations, no quotes.<|im_end|>\n"
            "<|im_start|>user\nTranslate from " + source + " to " + target + ":\n" + text +
            "<|im_end|>\n<|im_start|>assistant\n";

        std::vector<spk_token_id> toks(prompt.size() + 32);
        int32_t n = spk_tokenize(vocab, prompt.c_str(), (int32_t) prompt.size(),
                                 toks.data(), (int32_t) toks.size(), true, false);
        if (n <= 0) {
            spk_set_error(eng, "translation prompt did not tokenize");
            spk_sampler_free(smpl);
            return SPK_ERR_INTERNAL;
        }
        toks.resize((size_t) n);

        if ((int32_t) toks.size() >= (int32_t) spk_n_ctx(ctx) - 8) {
            spk_sampler_free(smpl);
            return SPK_ERR_CONTEXT_OVERFLOW;
        }

        spk_batch batch = spk_batch_init((int32_t) toks.size(), 0, 1);
        batch.n_tokens = (int32_t) toks.size();
        for (int32_t i = 0; i < (int32_t) toks.size(); ++i) {
            batch.token[i]      = toks[(size_t) i];
            batch.pos[i]        = i;
            batch.n_seq_id[i]   = 1;
            batch.seq_id[i][0]  = 0;
            batch.logits[i]     = (i == (int32_t) toks.size() - 1) ? 1 : 0;
        }
        const int32_t rc = spk_decode(ctx, batch);
        spk_batch_free(batch);
        if (rc != 0) {
            spk_set_error(eng, "translation prefill failed (%d)", rc);
            spk_sampler_free(smpl);
            return SPK_ERR_CONTEXT;
        }
        past = (spk_pos) toks.size();

        const int32_t max_tokens = (params != nullptr && params->max_tokens > 0)
            ? params->max_tokens : 256;
        if (spk_decode_loop(eng, ctx, model, smpl, &past, max_tokens, on_token, user_data) != 0) {
            status = eng->cancel.load() ? SPK_ERR_CANCELLED : SPK_ERR_CONTEXT;
        }
    }

    spk_sampler_free(smpl);
    spk_emit(on_token, user_data, nullptr, true);
    return status;
}
