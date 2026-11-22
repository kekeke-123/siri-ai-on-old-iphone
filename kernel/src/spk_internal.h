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
    // arch as reported by the runtime ("qwen35", "qwen2", ...), parsed lazily
    // from the model description; used for sampler defaults and diagnostics.
    std::string arch;

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
    // One decode at a time. llama_decode() is not reentrant on a context, and
    // two sessions used to race on the same one; the session mutex only guards
    // that session's bookkeeping, so the engine needs its own gate.
    std::mutex gen_mu;
    // error string is written from decode threads, so it gets its own lock
    // (never take mu here: spk_set_error is called while mu is held).
    std::mutex err_mu;
    bool       backend_up = false;
    bool       metal      = false;
    uint64_t   max_bytes  = 0;

    // Sequence slots. With n_seq_max > 1 each session owns its own runtime
    // sequence, so two conversations cannot overwrite each other's KV cache.
    int32_t    n_seq_max = 8;
    bool       seq_used[8] = {};
};

struct spk_session {
    spk_engine * eng = nullptr;
    std::string  system;
    std::vector<spk_msg> msgs;

    // runtime sequence this session owns; every batch, KV read/write and
    // snapshot is addressed by it instead of a hard-coded 0.
    int32_t seq = 0;

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

    // Sampling chains are not free: the penalties sampler allocates several
    // n_vocab sized arrays and walks the whole vocab. Reuse one chain per
    // session instead of rebuilding it on every turn.
    spk_sampler *  smpl      = nullptr;
    spk_gen_params smpl_cfg{};
    int32_t        smpl_vocab = 0;
};

// ---- shared helpers -------------------------------------------------

void spk_set_error(spk_engine * eng, const char * fmt, ...);

// sequence slot pool (engine.cpp)
int32_t spk_engine_alloc_seq(spk_engine * eng);
void    spk_engine_free_seq(spk_engine * eng, int32_t seq);

// batch size actually usable by this context (n_batch clamped to n_ctx)
int32_t spk_effective_batch(spk_engine * eng);

// last few llama.cpp log lines, joined by '|'; empty when nothing logged
void spk_log_tail(char * out, size_t len);

spk_slot * spk_slot_for(spk_engine * eng, spk_model_role role);
spk_status spk_slot_load(spk_engine * eng, spk_slot & slot);
void       spk_slot_unload(spk_slot & slot);

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
spk_sampler * spk_make_sampler(const spk_gen_params * p, int32_t n_vocab, const char * arch);

// architecture of the resident text model ("qwen35", "qwen2", "llama", ...).
// Empty string when no model is loaded. Cached after the first call.
const std::string & spk_engine_arch(spk_engine * eng);

// memory accounting
double spk_rss_mb();
