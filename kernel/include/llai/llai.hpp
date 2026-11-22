/*
 * llai.hpp - the C++ face of the LLAI kernel.
 *
 * The C ABI in spk.h stays the contract (it is what the iOS bridge and the
 * Windows import library bind to). This header is a thin, header-only RAII
 * wrapper over it: no new symbols, nothing to link beyond the kernel itself,
 * so every host - iOS, Windows, Linux - gets the same ergonomic API without
 * a second ABI to maintain.
 *
 *   #include "llai/llai.hpp"
 *
 *   auto engine = llai::Engine::open("qwen.gguf", {});
 *   if (!engine) { ... }
 *   llai::Session session(engine);
 *   session.stream("你好", {}, on_piece);
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "spk/spk.h"
#include "spk/spk_platform.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace llai {

// ------------------------------------------------------------------ status

class Status {
public:
    Status() = default;
    explicit Status(spk_status code) : code_(code) {}

    bool ok() const { return code_ == SPK_OK; }
    spk_status code() const { return code_; }
    const char * text() const { return spk_status_string(code_); }
    explicit operator bool() const { return ok(); }

private:
    spk_status code_ = SPK_OK;
};

// ------------------------------------------------------------------ config

struct Config {
    std::string model_path;
    std::string mmproj_path;          // empty = text only
    std::string chat_template;        // empty = what the GGUF carries
    std::string system_prompt;

    int  threads     = 0;             // 0 = platform decides
    int  context     = 0;             // 0 = derived from physical RAM
    int  batch       = 0;             // 0 = derived
    double max_model_gb = 0.0;        // 0 = 45% of RAM guard
    bool gpu         = true;          // weights on the GPU where it exists
    bool mmap        = true;
    bool secure      = true;          // escape markup, wipe KV on destroy
};

struct Reply {
    std::string text;
    Status      status;

    bool ok() const { return status.ok(); }
};

// ------------------------------------------------------------------ engine

class Engine {
public:
    Engine() = default;

    ~Engine() {
        if (h_ != nullptr) {
            spk_engine_destroy(h_);
            h_ = nullptr;
        }
    }

    Engine(Engine && other) noexcept : h_(other.h_) { other.h_ = nullptr; }

    Engine & operator=(Engine && other) noexcept {
        if (this != &other) {
            if (h_ != nullptr) {
                spk_engine_destroy(h_);
            }
            h_ = other.h_;
            other.h_ = nullptr;
        }
        return *this;
    }

    Engine(const Engine &) = delete;
    Engine & operator=(const Engine &) = delete;

    // Opens the weights. On failure the returned object is empty and `error`
    // (if given) carries the kernel's own message, not a generic one.
    static Engine open(const Config & cfg, std::string * error = nullptr) {
        spk_engine_config c{};
        c.model_path    = cfg.model_path.c_str();
        c.mmproj_path   = cfg.mmproj_path.empty() ? nullptr : cfg.mmproj_path.c_str();
        c.chat_template = cfg.chat_template.empty() ? nullptr : cfg.chat_template.c_str();
        c.system_prompt = cfg.system_prompt.empty() ? nullptr : cfg.system_prompt.c_str();
        c.n_threads     = cfg.threads;
        c.n_ctx         = cfg.context;
        c.n_batch       = cfg.batch;
        c.n_ubatch      = cfg.batch > 0 ? cfg.batch / 2 : 0;
        c.use_metal     = cfg.gpu ? 1 : 0;
        c.use_mmap      = cfg.mmap;
        c.secure_mode   = cfg.secure;
        c.max_model_gb  = cfg.max_model_gb;

        char buf[512] = {0};
        spk_engine * h = nullptr;
        if (spk_engine_create(&c, &h, buf, sizeof(buf)) != SPK_OK) {
            if (error != nullptr) {
                *error = buf[0] != '\0' ? std::string(buf) : std::string(spk_status_string(SPK_ERR_MODEL_LOAD));
            }
            return Engine();
        }
        Engine e;
        e.h_ = h;
        return e;
    }

    static Engine open(const std::string & model_path, std::string * error = nullptr) {
        Config cfg;
        cfg.model_path = model_path;
        return open(cfg, error);
    }

    bool valid() const { return h_ != nullptr; }
    explicit operator bool() const { return valid(); }

    spk_engine * handle() const { return h_; }

    std::string info_line() const {
        if (h_ == nullptr) {
            return std::string();
        }
        spk_info info{};
        if (spk_engine_info(h_, &info) != SPK_OK) {
            return std::string();
        }
        std::string s(info.model_desc);
        s += " | layers=";
        s += std::to_string(info.n_layer);
        s += " embd=";
        s += std::to_string(info.n_embd);
        s += " ctx=";
        s += std::to_string(info.n_ctx);
        return s;
    }

    spk_metrics metrics() const {
        spk_metrics m{};
        if (h_ != nullptr) {
            spk_engine_metrics(h_, &m);
        }
        return m;
    }

    void cancel() {
        if (h_ != nullptr) {
            spk_engine_cancel(h_);
        }
    }

    std::string last_error() const {
        if (h_ == nullptr) {
            return std::string();
        }
        const char * e = spk_engine_last_error(h_);
        return e != nullptr ? std::string(e) : std::string();
    }

private:
    spk_engine * h_ = nullptr;
};

// ----------------------------------------------------------------- session

struct Params {
    // Zero means "let the kernel pick". The sampler already knows that a
    // 0.8B Qwen wants top_k 20 / top_p 0.8 / repeat 1.05, while a generic model
    // wants 40 / 0.95 / 1.1; hard-coding one set here made every C++ host
    // inherit the wrong defaults and loop on small models.
    int    max_tokens    = 256;
    float  temperature   = 0.0f;   // 0 = default 0.7, negative = greedy
    float  top_p         = 0.0f;
    int    top_k         = 0;
    float  min_p         = 0.0f;
    float  repeat_penalty = 0.0f;
    int    repeat_last_n = 0;
    unsigned seed        = 0;      // 0 = random
};

class Session {
public:
    // A session borrows its engine; keep the engine alive while it exists.
    explicit Session(Engine & engine) : engine_(engine) {
        if (engine_.handle() == nullptr) {
            return;
        }
        spk_session * h = nullptr;
        if (spk_session_create(engine_.handle(), &h) == SPK_OK) {
            h_ = h;
        }
    }

    ~Session() {
        if (h_ != nullptr) {
            spk_session_destroy(h_);
            h_ = nullptr;
        }
    }

    // Factory matching the documented usage: `auto s = Session::create(engine);`
    // Returns nullptr when the engine has no free sequence slot.
    static std::unique_ptr<Session> create(Engine & engine) {
        std::unique_ptr<Session> s(new Session(engine));
        if (!s->valid()) {
            s.reset();
        }
        return s;
    }

    Session(const Session &) = delete;
    Session & operator=(const Session &) = delete;

    bool valid() const { return h_ != nullptr; }
    explicit operator bool() const { return valid(); }

    Status append(std::string_view role, std::string_view text) {
        if (h_ == nullptr) {
            return Status(SPK_ERR_INTERNAL);
        }
        return Status(spk_session_append(h_, std::string(role).c_str(),
                                        std::string(text).c_str(), nullptr));
    }

    void reset() {
        if (h_ != nullptr) {
            spk_session_reset(h_);
        }
    }

    void warmup() {
        if (h_ != nullptr) {
            spk_session_warmup(h_);
        }
    }

    int32_t n_past() const { return h_ != nullptr ? spk_session_n_past(h_) : 0; }

    // Streams the answer piece by piece. `last` marks the final flush.
    Status stream(std::string_view user_text, const Params & p,
                  std::function<void(std::string_view, bool)> on_piece) {
        if (h_ == nullptr) {
            return Status(SPK_ERR_INTERNAL);
        }
        Status st = append("user", user_text);
        if (!st.ok()) {
            return st;
        }
        spk_gen_params gp{};
        gp.max_tokens     = p.max_tokens;
        gp.temperature    = p.temperature;
        gp.top_p          = p.top_p;
        gp.top_k          = p.top_k;
        gp.min_p          = p.min_p;
        gp.repeat_penalty = p.repeat_penalty;
        gp.repeat_last_n  = p.repeat_last_n;
        gp.seed           = p.seed;

        struct sink {
            std::function<void(std::string_view, bool)> fn;
            std::string text;
        } s{std::move(on_piece), {}};

        auto thunk = [](const char * piece, bool is_final, void * ud) {
            auto * box = static_cast<sink *>(ud);
            if (piece != nullptr && piece[0] != '\0') {
                box->text.append(piece);
            }
            if (box->fn) {
                box->fn(std::string_view(piece != nullptr ? piece : ""), is_final);
            }
        };
        st = Status(spk_session_generate(h_, &gp, thunk, &s));
        // The kernel keeps the token history, not the message history: without
        // this, the next turn sees two user messages in a row and the model
        // answers as if the previous reply never happened.
        if (st.ok() && !s.text.empty() && h_ != nullptr) {
            spk_session_append(h_, "assistant", s.text.c_str(), nullptr);
        }
        return st;
    }

    Reply ask(std::string_view user_text, const Params & p = Params()) {
        std::string out;
        Status st = stream(user_text, p, [&out](std::string_view piece, bool) {
            out.append(piece.data(), piece.size());
        });
        Reply r;
        r.text = std::move(out);
        r.status = st;
        return r;
    }

private:
    Engine &      engine_;
    spk_session * h_ = nullptr;
};

// ---------------------------------------------------------------- platform

inline const char * version() { return spk_version(); }

inline const spk_platform_ops * platform() { return spk_platform_get_ops(); }

inline void set_platform(const spk_platform_ops * ops) { spk_platform_set_ops(ops); }

} // namespace llai
