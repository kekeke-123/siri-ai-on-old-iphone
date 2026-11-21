/*
 * spk_selftest - a headless smoke test for the Siri Plus Kernel.
 *
 * It lives outside the app so CI can prove, on a real GGUF, that the kernel
 * loads a model, prefills, decodes, snapshots and restores the sequence. It
 * only talks to the public C API, like any other embedder would.
 *
 *   spk_selftest <model.gguf> [mmproj.gguf]
 *
 * Exit code 0 means every stage passed. Anything else is a failure and the
 * operator gets one line per stage.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk/spk.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct capture {
    std::string text;
    int         tokens = 0;
};

void on_token(const char * piece, bool is_final, void * user_data) {
    capture * cap = static_cast<capture *>(user_data);
    if (piece != nullptr && piece[0] != '\0') {
        cap->text += piece;
        cap->tokens += 1;
    }
    (void) is_final;
}

bool stage(const char * name, bool ok, const std::string & detail) {
    std::printf("[%s] %-22s %s\n", ok ? " ok " : "fail", name, detail.c_str());
    return ok;
}

std::string first_line(const std::string & text, size_t limit = 160) {
    std::string out;
    for (char c : text) {
        if (c == '\n' || c == '\r') {
            break;
        }
        out.push_back(c);
        if (out.size() >= limit) {
            out += "...";
            break;
        }
    }
    return out;
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <model.gguf> [mmproj.gguf]\n", argv[0]);
        return 2;
    }

    const char * model_path  = argv[1];
    const char * mmproj_path = argc > 2 ? argv[2] : nullptr;
    const char * prompt      = std::getenv("SPK_SELFTEST_PROMPT");
    if (prompt == nullptr || prompt[0] == '\0') {
        prompt = "用一句话解释什么是本地推理。";
    }
    const int max_tokens = std::atoi(std::getenv("SPK_SELFTEST_TOKENS") ?: "32");
    const bool use_metal = std::strcmp(std::getenv("SPK_SELFTEST_METAL") ?: "0", "1") == 0;

    std::printf("== spk %s\n", spk_version());
    std::printf("== model %s\n", model_path);

    spk_engine_config cfg{};
    cfg.model_path     = model_path;
    cfg.mmproj_path    = mmproj_path;
    cfg.n_threads      = 4;
    cfg.n_ctx          = 1024;
    cfg.n_batch        = 256;
    cfg.n_ubatch       = 128;
    cfg.use_metal      = use_metal;
    cfg.use_mmap       = true;
    cfg.secure_mode    = false;
    cfg.max_model_gb   = 0.0;
    cfg.chat_template  = nullptr;
    cfg.system_prompt  = "你是一个简洁的中文语音助手，回答控制在两句话以内。";

    spk_engine * eng = nullptr;
    char err[512] = {0};

    bool ok = true;

    spk_status st = spk_engine_create(&cfg, &eng, err, sizeof(err));
    ok = stage("create", st == SPK_OK, st == SPK_OK ? "engine up" : err) && ok;
    if (st != SPK_OK) {
        return 1;
    }

    spk_info info{};
    st = spk_engine_info(eng, &info);
    ok = stage("info", st == SPK_OK,
               std::string("desc=") + info.model_desc +
               " layers=" + std::to_string(info.n_layer) +
               " embd=" + std::to_string(info.n_embd) +
               " ctx=" + std::to_string(info.n_ctx) +
               (info.recurrent ? " recurrent=1" : " recurrent=0")) && ok;

    spk_session * ses = nullptr;
    st = spk_session_create(eng, &ses);
    ok = stage("session", st == SPK_OK, st == SPK_OK ? "session up" : spk_engine_last_error(eng)) && ok;
    if (st != SPK_OK) {
        spk_engine_destroy(eng);
        return 1;
    }

    // warmup prefills and snapshots the system prompt
    st = spk_session_warmup(ses);
    ok = stage("warmup", st == SPK_OK || st == SPK_ERR_UNSUPPORTED,
               std::string(spk_status_string(st)) + " n_past=" + std::to_string(spk_session_n_past(ses))) && ok;

    // turn 1: plain generation
    st = spk_session_append(ses, "user", prompt, nullptr);
    capture cap;
    spk_gen_params gp{};
    gp.max_tokens = max_tokens;
    if (st == SPK_OK) {
        st = spk_session_generate(ses, &gp, on_token, &cap);
    }

    spk_metrics m{};
    spk_engine_metrics(eng, &m);

    const bool generated = st == SPK_OK && !cap.text.empty();
    ok = stage("generate", generated,
               std::string(spk_status_string(st)) +
               " ttft=" + std::to_string((int) m.ttft_ms) + "ms" +
               " decode=" + std::to_string((int) m.decode_tok_s) + "tok/s" +
               " n=" + std::to_string(cap.tokens) +
               " out=\"" + first_line(cap.text) + "\"") && ok;

    // turn 2: checkpoint / restore round trip. This is the part that breaks on
    // hybrid models (qwen35 keeps a recurrent state next to the KV cache), so
    // it is worth asserting instead of discovering it on a phone.
    st = spk_session_checkpoint(ses);
    const bool can_snapshot = st == SPK_OK;
    ok = stage("checkpoint", can_snapshot,
               can_snapshot ? "snapshot taken" : spk_status_string(st)) && ok;

    if (can_snapshot) {
        st = spk_session_restore(ses);
        ok = stage("restore", st == SPK_OK,
                   std::string(spk_status_string(st)) + " n_past=" + std::to_string(spk_session_n_past(ses))) && ok;

        st = spk_session_append(ses, "user", "再说一遍，短一点。", nullptr);
        capture cap2;
        spk_gen_params gp2 = gp;
        gp2.max_tokens = 16;
        if (st == SPK_OK) {
            st = spk_session_generate(ses, &gp2, on_token, &cap2);
        }
        ok = stage("restore+generate", st == SPK_OK && !cap2.text.empty(),
                   std::string(spk_status_string(st)) +
                   " out=\"" + first_line(cap2.text, 80) + "\"") && ok;
    }

    // cancel must be callable and must not wedge a later turn
    spk_engine_cancel(eng);
    st = spk_session_reset(ses);
    ok = stage("reset", st == SPK_OK, spk_status_string(st)) && ok;

    spk_session_destroy(ses);
    spk_engine_destroy(eng);

    std::printf("== %s\n", ok ? "SELFTEST PASS" : "SELFTEST FAIL");
    return ok ? 0 : 1;
}
