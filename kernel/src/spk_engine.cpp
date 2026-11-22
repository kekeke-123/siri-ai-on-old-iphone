/*
 * SPK engine: model loading, role slots, thermal policy and process pressure.
 *
 * The engine owns exactly one primary context. Vision, translation and ASR are
 * "slots" that are loaded on first use and dropped again under memory pressure,
 * which is the difference between a 4 GB iPhone being able to run a vision SLM
 * at all and the app being jetsam-killed on the second question.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_core.h"
#include "spk_internal.h"

#include <cmath>
#include <cstring>
#include <thread>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <unistd.h>
#endif

// ------------------------------------------------------------- logging

static std::mutex        g_log_mu;
static spk_log_cb        g_log_cb  = nullptr;
static void *            g_log_ud  = nullptr;

// llama.cpp explains the real reason for a failed decode (unsupported op, bad
// tensor shape, allocation failure) only on its own log channel. On iOS nothing
// is watching stderr, so the UI used to show a bare "-1". Keep the tail around
// and let the error message carry it.
static const int kLogTailLines = 5;
static const int kLogTailWidth = 200;
static char g_log_tail[kLogTailLines][kLogTailWidth];
static int  g_log_head = 0;

static void spk_log_trampoline(spk_log_level level, const char * text, void * /*ud*/) {
    std::lock_guard<std::mutex> lock(g_log_mu);

    if (text != nullptr) {
        char * slot = g_log_tail[g_log_head];
        strncpy(slot, text, (size_t) kLogTailWidth - 1);
        slot[kLogTailWidth - 1] = '\0';
        for (char * c = slot; *c != '\0'; ++c) {
            if (*c == '\n' || *c == '\r') { *c = '\0'; break; }
        }
        g_log_head = (g_log_head + 1) % kLogTailLines;
    }

    if (g_log_cb == nullptr || text == nullptr) {
        return;
    }
    g_log_cb((int32_t) level, text, g_log_ud);
}

void spk_log_tail(char * out, size_t len) {
    if (out == nullptr || len == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_log_mu);
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < kLogTailLines; ++i) {
        const int idx = (g_log_head + i) % kLogTailLines;
        const char * line = g_log_tail[idx];
        if (line[0] == '\0') {
            continue;
        }
        size_t n = strlen(line);
        if (used + n + 2 >= len) {
            break;
        }
        memcpy(out + used, line, n);
        used += n;
        out[used++] = '|';
    }
    if (used > 0) {
        out[used - 1] = '\0';
    }
}

void spk_engine_set_log_callback(spk_engine * eng, spk_log_cb cb, void * user_data) {
    if (eng == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_log_mu);
        g_log_cb = cb;
        g_log_ud = user_data;
    }
    eng->log_cb = cb;
    eng->log_ud = user_data;
    spk_log_set(spk_log_trampoline, nullptr);
}

// --------------------------------------------------------- hardware

static uint64_t spk_physical_ram() {
#if defined(__APPLE__)
    uint64_t mem = 0;
    size_t len = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) != 0) {
        return 0;
    }
    return mem;
#else
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page  = sysconf(_SC_PAGESIZE);
    if (pages <= 0 || page <= 0) {
        return 0;
    }
    return (uint64_t) pages * (uint64_t) page;
#endif
}

int32_t spk_auto_threads() {
    // Two cores are enough for a 1B SLM and leave the UI thread alone. On the
    // 6 core A-series parts we cap at 4: the efficiency cores make the graph
    // barrier slower, not faster, once the batch is already split four ways.
    int32_t hw = (int32_t) std::thread::hardware_concurrency();
    if (hw <= 0) {
        hw = 2;
    }
    if (hw > 4) {
        hw = 4;
    }
    return hw;
}

int32_t spk_auto_ctx(const spk_engine_config * cfg) {
    if (cfg != nullptr && cfg->n_ctx > 0) {
        return cfg->n_ctx;
    }
    const uint64_t ram = spk_physical_ram();
    if (ram == 0) {
        return 2048;
    }
    // KV cache is what grows with context. Budget roughly 96 MB of it: a 1B
    // model with 16 layers x 8 KV heads x 64 dims is ~ 1.5 MB per 512 tokens
    // at Q8, so 4096 tokens stays comfortably inside that budget.
    if (ram >= 6ull * 1024 * 1024 * 1024) {
        return 4096;
    }
    if (ram >= 4ull * 1024 * 1024 * 1024) {
        return 3072;
    }
    return 2048;
}

// ------------------------------------------------------------- slots

spk_slot * spk_slot_for(spk_engine * eng, spk_model_role role) {
    switch (role) {
        case SPK_MODEL_TEXT:      return &eng->text;
        case SPK_MODEL_VISION:    return &eng->vision;
        case SPK_MODEL_TRANSLATE: return &eng->translate;
        default:                  return nullptr;
    }
}

void spk_slot_unload(spk_slot & slot) {
    if (slot.vision != nullptr) {
        spk_vision_free(slot.vision);
        slot.vision = nullptr;
    }
    if (slot.ctx != nullptr) {
        if (slot.role == SPK_MODEL_TEXT) {
            // wipe the KV cache before releasing it: assistant turns can hold
            // whatever the user said, and the pages are reused by the next alloc
            spk_memory_clear(spk_get_memory(slot.ctx), true);
        }
        spk_free(slot.ctx);
        slot.ctx = nullptr;
    }
    if (slot.model != nullptr) {
        spk_model_free(slot.model);
        slot.model = nullptr;
    }
    slot.bytes    = 0;
    slot.n_ctx    = 0;
    slot.resident = false;
}

static spk_context_params spk_make_ctx_params(spk_engine * eng) {
    spk_context_params cp = spk_context_default_params();

    cp.n_ctx           = (uint32_t) spk_auto_ctx(&eng->cfg);
    cp.n_batch         = eng->cfg.n_batch  > 0 ? (uint32_t) eng->cfg.n_batch  : 256;
    cp.n_ubatch        = eng->cfg.n_ubatch > 0 ? (uint32_t) eng->cfg.n_ubatch : 128;
    cp.n_threads       = eng->cfg.n_threads > 0 ? eng->cfg.n_threads : spk_auto_threads();
    cp.n_threads_batch = cp.n_threads;

    cp.n_seq_max  = 1;
    cp.flash_attn_type = SPK_FLASH_DISABLED;

    // K/V stay f16 and flash attention stays off. Q8 K/V + FA is what made the
    // first decode fail on device (-1 at pos 0) while the exact same weights
    // decoded fine on the CPU path; Qwen3.5 keeps hybrid attention/SSM state
    // that the quantised cache path does not lay out correctly. Correctness
    // first: 3072 ctx at f16 is ~40 MB, not worth a model that cannot answer.
    cp.type_k = SPK_TYPE_F16;
    cp.type_v = SPK_TYPE_F16;

    cp.offload_kqv = eng->metal;
    cp.no_perf     = false;
    return cp;
}

spk_status spk_slot_load(spk_engine * eng, spk_slot & slot) {
    if (slot.resident) {
        return SPK_OK;
    }
    if (slot.path.empty()) {
        spk_set_error(eng, "slot %d has no model attached", (int) slot.role);
        return SPK_ERR_NOT_FOUND;
    }

    std::string why;
    const spk_status guard = spk_guard_model_file(slot.path.c_str(), eng->max_bytes, &why);
    if (guard != SPK_OK) {
        spk_set_error(eng, "guard rejected %s: %s", slot.path.c_str(), why.c_str());
        return guard;
    }

    // vision and translation borrow the primary model's vocab/backend, so the
    // text model has to be up first
    if (slot.role != SPK_MODEL_TEXT) {
        spk_status st = spk_slot_load(eng, eng->text);
        if (st != SPK_OK) {
            return st;
        }
    }

    spk_model_params mp = spk_model_default_params();
    mp.n_gpu_layers     = eng->cfg.use_metal ? 999 : 0;
    mp.load_mode        = eng->cfg.use_mmap ? SPK_LOAD_MMAP : SPK_LOAD_NONE;
    mp.use_extra_bufts  = true;     // repacked quantization for the NEON kernels
    mp.check_tensors    = false;    // full validation costs 20+ s on device

    const uint64_t t0 = (uint64_t) spk_time_us();
    spk_model * model = spk_model_load(slot.path.c_str(), mp);
    if (model == nullptr) {
        spk_set_error(eng, "spk_model_load failed for %s", slot.path.c_str());
        return SPK_ERR_MODEL_LOAD;
    }

    slot.model = model;
    slot.bytes = spk_model_size(model);

    if (slot.role == SPK_MODEL_VISION) {
        spk_vision_params tp = spk_vision_params_default();
        tp.use_gpu       = eng->metal;
        tp.n_threads     = eng->cfg.n_threads > 0 ? eng->cfg.n_threads : spk_auto_threads();
        tp.print_timings = false;
        tp.warmup        = false;
        tp.flash_attn_type = SPK_FLASH_AUTO;
        // cap the image tokens so a 4032x3024 photo cannot blow the context
        tp.image_max_tokens = 1024;

        slot.vision = spk_vision_init(slot.aux.c_str(), model, tp);
        if (slot.vision == nullptr) {
            spk_set_error(eng, "spk_vision_init failed for %s", slot.aux.c_str());
            spk_slot_unload(slot);
            return SPK_ERR_MODEL_LOAD;
        }
        if (!spk_vision_support(slot.vision)) {
            spk_set_error(eng, "mmproj does not support vision input");
            spk_slot_unload(slot);
            return SPK_ERR_VISION_UNAVAILABLE;
        }
    } else {
        spk_context_params cp = spk_make_ctx_params(eng);
        slot.ctx = spk_init_from_model(model, cp);
        if (slot.ctx == nullptr) {
            spk_set_error(eng, "spk_init_from_model failed for %s", slot.path.c_str());
            spk_slot_unload(slot);
            return SPK_ERR_CONTEXT;
        }
        slot.n_ctx = (int32_t) spk_n_ctx(slot.ctx);
    }

    slot.resident = true;
    eng->metrics.peak_rss_mb = std::fmax(eng->metrics.peak_rss_mb, spk_rss_mb());

    const uint64_t dt_us = (uint64_t) spk_time_us() - t0;
    (void) dt_us;
    return SPK_OK;
}

// ------------------------------------------------------------ engine

spk_status spk_engine_create(const spk_engine_config * cfg, spk_engine ** out,
                             char * err_buf, size_t err_len) {
    if (out == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    *out = nullptr;
    if (err_buf != nullptr && err_len > 0) {
        err_buf[0] = '\0';
    }
    if (cfg == nullptr || cfg->model_path == nullptr) {
        if (err_buf != nullptr && err_len > 0) {
            snprintf(err_buf, err_len, "model_path is required");
        }
        return SPK_ERR_INVALID_ARGUMENT;
    }

    auto * eng = new spk_engine();
    eng->cfg = *cfg;

    // Prompt ingestion is the one place where a bigger batch is pure win: a
    // 512 token slab halves the number of Metal graph launches for long system
    // prompts. Devices with 6 GB or more can afford the extra scratch memory.
    if (eng->cfg.n_batch <= 0 || eng->cfg.n_ubatch <= 0) {
        const bool big = spk_physical_ram() >= 6ull * 1024ull * 1024ull * 1024ull;
        if (eng->cfg.n_batch  <= 0) eng->cfg.n_batch  = big ? 512 : 256;
        if (eng->cfg.n_ubatch <= 0) eng->cfg.n_ubatch = big ? 256 : 128;
    }

    eng->metal = cfg->use_metal;

    // one time backend init; cheap and idempotent from our side
    static std::once_flag once;
    std::call_once(once, [] {
        spk_backend_init();
    });
    eng->backend_up = true;

    if (eng->metal && !spk_supports_gpu_offload()) {
        eng->metal = false;   // build without Metal, silently fall back to CPU
    }

    const uint64_t ram = spk_physical_ram();
    if (cfg->max_model_gb > 0.0) {
        eng->max_bytes = (uint64_t) (cfg->max_model_gb * 1024.0 * 1024.0 * 1024.0);
    } else if (ram > 0) {
        // iOS gives a foreground app roughly 45-50% of physical RAM before jetsam
        eng->max_bytes = (uint64_t) ((double) ram * 0.45);
    }

    char resolved[1024];
    if (!spk_guard_path(cfg->model_path, resolved, sizeof(resolved))) {
        spk_set_error(eng, "model path rejected: %s", cfg->model_path);
        if (err_buf) snprintf(err_buf, err_len, "model path rejected");
        delete eng;
        return SPK_ERR_MODEL_REJECTED;
    }

    eng->text.role = SPK_MODEL_TEXT;
    eng->text.path = resolved;
    eng->vision.role = SPK_MODEL_VISION;
    eng->translate.role = SPK_MODEL_TRANSLATE;

    if (cfg->mmproj_path != nullptr && cfg->mmproj_path[0] != '\0') {
        char aux[1024];
        if (spk_guard_path(cfg->mmproj_path, aux, sizeof(aux))) {
            eng->vision.aux = aux;
            eng->mmproj_path = aux;
        } else {
            spk_set_error(eng, "mmproj path rejected: %s", cfg->mmproj_path);
        }
    }

    if (cfg->chat_template != nullptr) {
        eng->chat_template = cfg->chat_template;
    }
    if (cfg->system_prompt != nullptr) {
        eng->system_prompt = cfg->system_prompt;
    }

    std::string why;
    spk_status st = spk_guard_model_file(eng->text.path.c_str(), eng->max_bytes, &why);
    if (st != SPK_OK) {
        spk_set_error(eng, "guard rejected model: %s", why.c_str());
        if (err_buf) snprintf(err_buf, err_len, "%s", why.c_str());
        delete eng;
        return st;
    }

    {
        std::lock_guard<std::mutex> lock(eng->mu);
        st = spk_slot_load(eng, eng->text);
    }
    if (st != SPK_OK) {
        const std::string e = eng->err;
        if (err_buf) snprintf(err_buf, err_len, "%s", e.c_str());
        spk_engine_destroy(eng);
        return st;
    }

    // a model that cannot hold a conversation is not worth a second thought
    if (spk_model_has_encoder(eng->text.model) && !spk_model_has_decoder(eng->text.model)) {
        spk_set_error(eng, "encoder-only model cannot be used as the primary SLM");
        spk_engine_destroy(eng);
        return SPK_ERR_MODEL_REJECTED;
    }

    *out = eng;
    return SPK_OK;
}

void spk_engine_destroy(spk_engine * eng) {
    if (eng == nullptr) {
        return;
    }
    eng->cancel.store(true);
    {
        std::lock_guard<std::mutex> lock(eng->mu);
        spk_slot_unload(eng->translate);
        spk_slot_unload(eng->vision);
        spk_slot_unload(eng->text);
    }
    delete eng;
}

const char * spk_engine_last_error(const spk_engine * eng) {
    if (eng == nullptr) {
        return "";
    }
    return eng->err.c_str();
}

const std::string & spk_engine_arch(spk_engine * eng) {
    static const std::string none;
    if (eng == nullptr || eng->text.model == nullptr) {
        return none;
    }
    if (!eng->arch.empty()) {
        return eng->arch;
    }
    char desc[128] = {0};
    spk_model_desc(eng->text.model, desc, sizeof(desc));
    // "<arch> <size> <quant> - <type>", so the first token is the arch name
    std::string text(desc);
    const size_t sp = text.find(' ');
    eng->arch = (sp == std::string::npos) ? text : text.substr(0, sp);
    if (eng->arch.empty()) {
        eng->arch = "unknown";
    }
    return eng->arch;
}

spk_status spk_engine_info(const spk_engine * eng, spk_info * out) {
    if (eng == nullptr || out == nullptr || eng->text.model == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    memset(out, 0, sizeof(*out));
    out->model_bytes = spk_model_size(eng->text.model);
    out->params      = spk_model_n_params(eng->text.model);
    out->n_ctx       = eng->text.ctx ? (int32_t) spk_n_ctx(eng->text.ctx) : 0;
    out->n_ctx_train = spk_model_n_ctx_train(eng->text.model);
    out->n_threads   = eng->cfg.n_threads > 0 ? eng->cfg.n_threads : spk_auto_threads();
    out->n_layer     = spk_model_n_layer(eng->text.model);
    out->n_embd      = spk_model_n_embd(eng->text.model);
    out->vision      = eng->vision.vision != nullptr;
    out->metal       = eng->metal;
    out->recurrent   = spk_model_is_recurrent(eng->text.model);
    spk_model_desc(eng->text.model, out->model_desc, sizeof(out->model_desc));
    return SPK_OK;
}

spk_status spk_engine_metrics(const spk_engine * eng, spk_metrics * out) {
    if (eng == nullptr || out == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    *out = eng->metrics;
    out->thermal_state = eng->thermal.load();
    out->peak_rss_mb = std::fmax(out->peak_rss_mb, spk_rss_mb());
    return SPK_OK;
}

spk_status spk_engine_add_model(spk_engine * eng, spk_model_role role,
                                const char * path, const char * aux_path) {
    if (eng == nullptr || path == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    if (role == SPK_MODEL_ASR) {
        char resolved[1024];
        if (!spk_guard_path(path, resolved, sizeof(resolved))) {
            spk_set_error(eng, "asr path rejected");
            return SPK_ERR_MODEL_REJECTED;
        }
        eng->asr_path = resolved;
        return SPK_OK;
    }

    spk_slot * slot = spk_slot_for(eng, role);
    if (slot == nullptr || role == SPK_MODEL_TEXT) {
        return SPK_ERR_INVALID_ARGUMENT;
    }

    char resolved[1024];
    if (!spk_guard_path(path, resolved, sizeof(resolved))) {
        spk_set_error(eng, "path rejected for slot %d", (int) role);
        return SPK_ERR_MODEL_REJECTED;
    }

    std::lock_guard<std::mutex> lock(eng->mu);
    if (slot->resident) {
        spk_slot_unload(*slot);
    }
    slot->path = resolved;
    slot->aux.clear();

    if (role == SPK_MODEL_VISION) {
        if (aux_path == nullptr) {
            spk_set_error(eng, "vision slot needs an mmproj path");
            return SPK_ERR_INVALID_ARGUMENT;
        }
        char aux[1024];
        if (!spk_guard_path(aux_path, aux, sizeof(aux))) {
            spk_set_error(eng, "mmproj path rejected");
            return SPK_ERR_MODEL_REJECTED;
        }
        slot->aux = aux;
    }
    return SPK_OK;
}

spk_status spk_engine_set_thermal_state(spk_engine * eng, spk_thermal_state state) {
    if (eng == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    eng->thermal.store((int) state);

    // Throttle by shrinking the batch, not by lowering the thread count: fewer
    // threads just moves the heat to fewer cores and makes latency spiky.
    switch (state) {
        case SPK_THERMAL_SERIOUS:
            eng->cfg.n_batch  = 128;
            eng->cfg.n_ubatch = 64;
            break;
        case SPK_THERMAL_CRITICAL:
            eng->cfg.n_batch  = 64;
            eng->cfg.n_ubatch = 32;
            break;
        default:
            break;
    }
    return SPK_OK;
}

spk_status spk_engine_handle_memory_warning(spk_engine * eng) {
    if (eng == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    if (!eng->mu.try_lock()) {
        // busy generating, the OS will warn again
        return SPK_OK;
    }
    spk_slot_unload(eng->translate);
    spk_slot_unload(eng->vision);
    eng->mu.unlock();
    return SPK_OK;
}

void spk_engine_cancel(spk_engine * eng) {
    if (eng != nullptr) {
        eng->cancel.store(true);
    }
}
