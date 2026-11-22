/*
 * spk_cli - the command line face of the LLAI Kernel.
 *
 * The same binary runs on Windows (spk.exe / LLAI.exe), Linux and macOS,
 * because the kernel underneath is portable C++ and gets its device facts from
 * the platform layer instead of from iOS. Nothing here talks to an Apple
 * framework.
 *
 *   spk --model qwen.gguf                    interactive chat on stdin
 *   spk -m qwen.gguf -p "你好" -n 128         one shot, answer to stdout
 *   echo 你好 | spk -m qwen.gguf             piped one shot
 *
 * On Windows: run `chcp 65001` first if the console mangles Chinese output.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk/spk.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

struct options {
    std::string model;
    std::string mmproj;
    std::string prompt;
    std::string system;
    int         max_tokens = 256;
    int         ctx        = 0;      /* 0 = auto from RAM */
    int         threads    = 0;      /* 0 = auto from platform */
    float       temp       = 0.7f;
    float       top_p      = 0.9f;
    bool        quiet      = false;
    bool        no_warmup  = false;
};

void usage(const char * argv0) {
    std::printf(
        "LLAI Kernel CLI %s\n\n"
        "usage: %s --model <path.gguf> [options]\n\n"
        "  -m, --model <path>     GGUF weights (required)\n"
        "      --mmproj <path>    vision projection, optional\n"
        "  -p, --prompt <text>    one shot question; omit for interactive chat\n"
        "  -n, --n <tokens>       max tokens to generate (default 256)\n"
        "      --ctx <tokens>     context size (default: from available RAM)\n"
        "  -t, --threads <n>      decoding threads (default: performance cores)\n"
        "      --temp <f>         temperature (default 0.7)\n"
        "      --top-p <f>        nucleus sampling (default 0.9)\n"
        "      --system <text>    system prompt\n"
        "      --no-warmup        skip the prefill of the system prompt\n"
        "      --quiet            no stats on stderr\n"
        "  -h, --help             this text\n\n"
        "interactive: type a question per line, /reset clears the context,\n"
        "             /exit quits.\n",
        spk_version(), argv0);
}

bool take_value(int argc, char ** argv, int * i, std::string * out) {
    if (*i + 1 >= argc) {
        return false;
    }
    *out = argv[++(*i)];
    return true;
}

bool parse(int argc, char ** argv, options * opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        std::string v;
        if (a == "-h" || a == "--help") {
            usage(argv[0]);
            std::exit(0);
        } else if (a == "-m" || a == "--model") {
            if (!take_value(argc, argv, &i, &opt->model)) return false;
        } else if (a == "--mmproj") {
            if (!take_value(argc, argv, &i, &opt->mmproj)) return false;
        } else if (a == "-p" || a == "--prompt") {
            if (!take_value(argc, argv, &i, &opt->prompt)) return false;
        } else if (a == "--system") {
            if (!take_value(argc, argv, &i, &opt->system)) return false;
        } else if (a == "-n" || a == "--n") {
            if (!take_value(argc, argv, &i, &v)) return false;
            opt->max_tokens = std::atoi(v.c_str());
        } else if (a == "--ctx") {
            if (!take_value(argc, argv, &i, &v)) return false;
            opt->ctx = std::atoi(v.c_str());
        } else if (a == "-t" || a == "--threads") {
            if (!take_value(argc, argv, &i, &v)) return false;
            opt->threads = std::atoi(v.c_str());
        } else if (a == "--temp") {
            if (!take_value(argc, argv, &i, &v)) return false;
            opt->temp = (float) std::atof(v.c_str());
        } else if (a == "--top-p") {
            if (!take_value(argc, argv, &i, &v)) return false;
            opt->top_p = (float) std::atof(v.c_str());
        } else if (a == "--no-warmup") {
            opt->no_warmup = true;
        } else if (a == "--quiet") {
            opt->quiet = true;
        } else {
            std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
            return false;
        }
    }
    return !opt->model.empty();
}

void on_piece(const char * piece, bool is_final, void * /*ud*/) {
    if (piece != nullptr && piece[0] != '\0') {
        std::fputs(piece, stdout);
        std::fflush(stdout);
    }
    (void) is_final;
}

} // namespace

int main(int argc, char ** argv) {
#if defined(_WIN32)
    // The kernel speaks UTF-8 everywhere; the console has to agree.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    options opt;
    if (!parse(argc, argv, &opt)) {
        usage(argv[0]);
        return 2;
    }

    spk_engine_config cfg{};
    cfg.model_path    = opt.model.c_str();
    cfg.mmproj_path   = opt.mmproj.empty() ? nullptr : opt.mmproj.c_str();
    cfg.n_threads     = opt.threads;
    cfg.n_ctx         = opt.ctx;
    cfg.n_batch       = 0;
    cfg.n_ubatch      = 0;
    cfg.use_metal     = 0;   /* set 1 only on a host with a verified GPU path */
    cfg.use_mmap      = true;
    cfg.secure_mode   = false;
    cfg.max_model_gb  = 0.0;
    cfg.chat_template = nullptr;
    cfg.system_prompt = opt.system.empty() ? nullptr : opt.system.c_str();

    char err[512] = {0};
    spk_engine * eng = nullptr;
    spk_status st = spk_engine_create(&cfg, &eng, err, sizeof(err));
    if (st != SPK_OK) {
        std::fprintf(stderr, "engine: %s\n", err[0] != '\0' ? err : spk_status_string(st));
        return 1;
    }

    spk_info info{};
    if (spk_engine_info(eng, &info) == SPK_OK && !opt.quiet) {
        std::fprintf(stderr, "kernel %s | %s | layers=%d embd=%d ctx=%d%s\n",
                     spk_version(), info.model_desc, (int) info.n_layer, (int) info.n_embd,
                     (int) info.n_ctx, info.recurrent ? " recurrent" : "");
    }

    spk_session * ses = nullptr;
    st = spk_session_create(eng, &ses);
    if (st != SPK_OK) {
        std::fprintf(stderr, "session: %s\n", spk_engine_last_error(eng));
        spk_engine_destroy(eng);
        return 1;
    }

    if (!opt.no_warmup) {
        st = spk_session_warmup(ses);
        if (st != SPK_OK && st != SPK_ERR_UNSUPPORTED) {
            std::fprintf(stderr, "warmup: %s\n", spk_engine_last_error(eng));
        }
    }

    spk_gen_params gp{};
    gp.max_tokens  = opt.max_tokens;
    gp.temperature = opt.temp;
    gp.top_p       = opt.top_p;

    bool ok = true;
    bool interactive = opt.prompt.empty();

    do {
        std::string line = opt.prompt;
        if (interactive) {
            std::fputs("> ", stdout);
            std::fflush(stdout);
            if (!std::getline(std::cin, line)) {
                break;
            }
            if (line == "/exit" || line == "/quit" || line == "exit" || line == "quit") {
                break;
            }
            if (line == "/reset") {
                spk_session_reset(ses);
                spk_session_warmup(ses);
                std::fputs("[context cleared]\n", stderr);
                continue;
            }
            if (line.empty()) {
                continue;
            }
        }

        st = spk_session_append(ses, "user", line.c_str(), nullptr);
        if (st != SPK_OK) {
            std::fprintf(stderr, "append: %s\n", spk_engine_last_error(eng));
            ok = false;
            break;
        }

        st = spk_session_generate(ses, &gp, on_piece, nullptr);
        std::printf("\n");

        if (st != SPK_OK) {
            std::fprintf(stderr, "generate: %s\n", spk_engine_last_error(eng));
            ok = false;
        }

        if (!opt.quiet) {
            spk_metrics m{};
            spk_engine_metrics(eng, &m);
            std::fprintf(stderr, "[%lld tok | ttft %.0f ms | %.2f tok/s]\n",
                         (long long) m.n_decode_tokens, m.ttft_ms, m.decode_tok_s);
        }

        if (!interactive) {
            break;
        }
    } while (true);

    spk_session_destroy(ses);
    spk_engine_destroy(eng);
    return ok ? 0 : 1;
}
