// SPK internal definitions. This header is deliberately free of upstream
// identifiers: everything here is spk_* (see spk_core.h for the single
// adapter unit that owns the upstream names).
#pragma once

#include "spk.h"

#include "spk_core.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

struct spk_msg {
    std::string role;
    std::string text;
    std::vector<std::string> images;
};

struct spk_slot {
    spk_model_role role = SPK_MODEL_TEXT;
    std::string path;
    std::string aux;
    spk_model *   model = nullptr;
    spk_context * ctx   = nullptr;
    spk_vision_ctx * vision = nullptr;
    uint64_t      bytes = 0;
    int32_t       n_ctx = 0;
    bool          resident = false;
};

struct spk_engine {
    spk_engine_config cfg{};
    std::string model_path;
    std::string mmproj_path;
    std::string chat_template;
    std::string system_prompt;
    std::string asr_path;
    std::string err;

    spk_slot text;
    spk_slot vision;
    spk_slot translate;

    spk_host_asr_cb host_asr    = nullptr;
    void *          host_asr_ud = nullptr;

    std::atomic<bool> cancel{false};
    std::atomic<int>  thermal{SPK_THERMAL_NOMINAL};

    spk_metrics metrics{};
    spk_log_cb  log_cb  = nullptr;
    void *      log_ud  = nullptr;

    std::mutex mu;
    bool       backend_up = false;
    bool       metal      = false;
    uint64_t   max_bytes  = 0;
};

struct spk_session {
    spk_engine * eng = nullptr;
    std::string  system;
    std::vector<spk_msg> msgs;

    std::vector<spk_token_id> tokens;
    spk_pos n_past = 0;

    // KV snapshot of the system prompt only
    std::vector<uint8_t> prefix;
    std::vector<spk_token_id> prefix_tokens;

    // full sequence snapshot (explicit checkpoint/restore)
    std::vector<uint8_t> checkpoint;
    spk_pos checkpoint_len = 0;

    spk_metrics last{};
    std::mutex  mu;
};

// ---- shared helpers -------------------------------------------------

void spk_set_error(spk_engine * eng, const char * fmt, ...);

spk_slot * spk_slot_for(spk_engine * eng, spk_model_role role);
spk_status spk_slot_load(spk_engine * eng, spk_slot & slot);
void       spk_slot_unload(spk_slot & slot);

// Single token-fanout point shared by generation, translation and vision.
// Drop-in safe for a null callback, which is how the non-streaming callers
// ask for "just collect the text".
inline void spk_emit(spk_token_cb cb, void * ud, const char * piece, bool is_final) {
    if (cb != nullptr) {
        cb(piece, is_final, ud);
    }
}

std::string spk_apply_template(spk_engine * eng, const std::vector<spk_msg> & msgs, bool add_ass);

int32_t spk_auto_threads();
int32_t spk_auto_ctx(const spk_engine_config * cfg);

// guard
bool        spk_guard_path(const char * path, char * out, size_t out_len);
spk_status  spk_guard_model_file(const char * path, uint64_t max_bytes, std::string * why);
std::string spk_guard_escape_markup(const std::string & text);
bool        spk_guard_has_markup(const std::string & text);

// vision
spk_status spk_vision_eval(spk_engine * eng, spk_session * ses,
                           const std::string & prompt,
                           const std::vector<std::string> & images,
                           spk_pos n_past, bool logits_last, spk_pos * new_n_past);

// sampling
spk_sampler * spk_make_sampler(const spk_gen_params * p, int32_t n_vocab);

// memory accounting
double spk_rss_mb();
