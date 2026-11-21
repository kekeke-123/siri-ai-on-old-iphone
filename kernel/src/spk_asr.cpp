/*
 * SPK audio path. Transcription itself is a host capability: on iOS the app
 * hands us an Apple Speech recogniser (fully on device), and any local audio
 * SLM can be dropped in behind the same callback without touching the kernel.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <cstring>

void spk_asr_set_host_callback(spk_engine * eng, spk_host_asr_cb cb, void * user_data) {
    if (eng == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(eng->mu);
    eng->host_asr    = cb;
    eng->host_asr_ud = user_data;
}

bool spk_supports_asr(const spk_engine * eng) {
    return eng != nullptr && (eng->host_asr != nullptr || !eng->asr_path.empty());
}

spk_status spk_asr_transcribe(spk_engine * eng, const char * audio_path,
                              const char * language, char * out, size_t out_len) {
    if (eng == nullptr || out == nullptr || out_len == 0) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    out[0] = '\0';

    spk_host_asr_cb cb = nullptr;
    void * ud = nullptr;
    {
        std::lock_guard<std::mutex> lock(eng->mu);
        cb = eng->host_asr;
        ud = eng->host_asr_ud;
    }

    if (cb == nullptr) {
        return SPK_ERR_ASR_UNAVAILABLE;
    }

    char * text = cb(audio_path, language, ud);
    if (text == nullptr) {
        return SPK_ERR_ASR_UNAVAILABLE;
    }

    const size_t n = strlen(text);
    if (n >= out_len) {
        free(text);
        return SPK_ERR_INVALID_ARGUMENT;
    }
    memcpy(out, text, n + 1);
    free(text);
    return SPK_OK;
}
