/*
 * Siri Plus Kernel (SPK) - public C API
 *
 * A thin, embeddable inference runtime built on a heavily modified llama.cpp.
 * Everything here is safe to call from Swift/Objective-C through SPKBridge,
 * or directly from C/C++ code.
 *
 * Design rules:
 *   - no global state, one spk_engine is one isolated runtime
 *   - every function returns spk_status, the message lives in the engine
 *   - callbacks never retain ownership of the buffers they receive
 *   - all strings are UTF-8, all sizes are bytes unless stated otherwise
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SPK_H
#define SPK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPK_VERSION_MAJOR 1
#define SPK_VERSION_MINOR 4
#define SPK_VERSION_PATCH 0
#define SPK_VERSION_STRING "1.4.0"

/* ------------------------------------------------------------------ */
/* status codes                                                        */
/* ------------------------------------------------------------------ */

typedef enum spk_status {
    SPK_OK                    = 0,
    SPK_ERR_INVALID_ARGUMENT  = 1,
    SPK_ERR_NOT_FOUND         = 2,
    SPK_ERR_UNSUPPORTED       = 3,
    SPK_ERR_MODEL_REJECTED    = 4,   /* guard rejected the file before loading  */
    SPK_ERR_MODEL_LOAD        = 5,
    SPK_ERR_CONTEXT           = 6,
    SPK_ERR_OUT_OF_MEMORY     = 7,
    SPK_ERR_CONTEXT_OVERFLOW  = 8,
    SPK_ERR_CANCELLED         = 9,
    SPK_ERR_THERMAL           = 10,
    SPK_ERR_INTERNAL          = 11,
    SPK_ERR_VISION_UNAVAILABLE= 12,
    SPK_ERR_ASR_UNAVAILABLE   = 13,
} spk_status;

/* ------------------------------------------------------------------ */
/* model roles                                                         */
/* ------------------------------------------------------------------ */

typedef enum spk_model_role {
    SPK_MODEL_TEXT      = 0,  /* the main SLM, always resident            */
    SPK_MODEL_VISION    = 1,  /* aux_path carries the mmproj (vision SLM) */
    SPK_MODEL_TRANSLATE = 2,  /* dedicated translation SLM (T5 family etc.)*/
    SPK_MODEL_ASR       = 3,  /* optional whisper.cpp ggml binary          */
    SPK_MODEL_ROLE_COUNT = 4,
} spk_model_role;

typedef enum spk_thermal_state {
    SPK_THERMAL_NOMINAL   = 0,
    SPK_THERMAL_FAIR      = 1,
    SPK_THERMAL_SERIOUS   = 2,
    SPK_THERMAL_CRITICAL  = 3,
} spk_thermal_state;

/* ------------------------------------------------------------------ */
/* config                                                              */
/* ------------------------------------------------------------------ */

typedef struct spk_engine_config {
    const char * model_path;      /* required, absolute path inside the sandbox */
    const char * mmproj_path;     /* optional, enables the vision SLM          */
    int32_t n_threads;            /* 0 = auto (performance cores)              */
    int32_t n_ctx;                /* 0 = auto from device memory               */
    int32_t n_batch;              /* 0 = 512                                   */
    int32_t n_ubatch;             /* 0 = 128                                   */
    bool    use_metal;            /* default true when a Metal device exists   */
    bool    use_mmap;             /* default true                              */
    bool    secure_mode;          /* wipe KV on session destroy, escape markup */
    double  max_model_gb;         /* 0 = auto DoS guard, else hard cap          */
    const char * chat_template;   /* optional override, NULL = from GGUF       */
    const char * system_prompt;   /* optional default system prompt            */
} spk_engine_config;

typedef struct spk_info {
    uint64_t model_bytes;
    uint64_t params;
    int32_t  n_ctx;
    int32_t  n_ctx_train;
    int32_t  n_threads;
    int32_t  n_layer;
    int32_t  n_embd;
    bool     vision;
    bool     metal;
    bool     recurrent;
    char     model_desc[128];
} spk_info;

typedef struct spk_metrics {
    double  ttft_ms;        /* time to first token of the last generation */
    double  prefill_tok_s;
    double  decode_tok_s;
    int64_t n_prefill_tokens;
    int64_t n_decode_tokens;
    double  peak_rss_mb;
    int32_t thermal_state;
} spk_metrics;

typedef struct spk_gen_params {
    int32_t max_tokens;     /* 0 = 256                                        */
    int32_t top_k;          /* 0 = 40                                         */
    float   top_p;          /* 0 = 0.95                                       */
    float   min_p;          /* 0 = 0.05                                       */
    float   temperature;    /* 0 = 0.7                                        */
    float   repeat_penalty; /* 0 = 1.1                                       */
    int32_t repeat_last_n;  /* 0 = 64                                         */
    int32_t seed;           /* 0 = random                                     */
} spk_gen_params;

/* token stream callback. piece is NUL terminated and only valid during the call */
typedef void (*spk_token_cb)(const char * piece, bool is_final, void * user_data);
/* log callback, level follows ggml_log_level (0=info 1=warn 2=error) */
typedef void (*spk_log_cb)(int32_t level, const char * message, void * user_data);
/* host side ASR fallback (Apple Speech / any engine the app already owns) */
typedef char * (*spk_host_asr_cb)(const char * audio_path, const char * language, void * user_data);

/* ------------------------------------------------------------------ */
/* engine lifecycle                                                    */
/* ------------------------------------------------------------------ */

typedef struct spk_engine spk_engine;

const char * spk_version(void);
const char * spk_status_string(spk_status st);

spk_status spk_engine_create(const spk_engine_config * cfg,
                             spk_engine ** out_engine,
                             char * err_buf, size_t err_len);

void spk_engine_destroy(spk_engine * engine);

/* human readable error of the last failed call, "" when the last call was ok */
const char * spk_engine_last_error(const spk_engine * engine);

spk_status spk_engine_info(const spk_engine * engine, spk_info * out);
spk_status spk_engine_metrics(const spk_engine * engine, spk_metrics * out);

/* attach the extra models. Loading is lazy: nothing is read until first use. */
spk_status spk_engine_add_model(spk_engine * engine, spk_model_role role,
                                const char * path, const char * aux_path);

/* 0..3, from ProcessInfo.thermalState. Generation slows down instead of being killed. */
spk_status spk_engine_set_thermal_state(spk_engine * engine, spk_thermal_state state);

/* called from didReceiveMemoryWarning / OS pressure source: drops every non-primary model */
spk_status spk_engine_handle_memory_warning(spk_engine * engine);

/* cancel whatever is generating right now (thread safe, returns immediately) */
void spk_engine_cancel(spk_engine * engine);

void spk_engine_set_log_callback(spk_engine * engine, spk_log_cb cb, void * user_data);

/* ------------------------------------------------------------------ */
/* sessions                                                            */
/* ------------------------------------------------------------------ */

typedef struct spk_session spk_session;

spk_status spk_session_create(spk_engine * engine, spk_session ** out_session);
void       spk_session_destroy(spk_session * session);
spk_status spk_session_reset(spk_session * session);

/* set/replace the system prompt of this session (invalidates the prefix snapshot) */
spk_status spk_session_set_system(spk_session * session, const char * system_prompt);

/* append one turn. images may be NULL; otherwise a NULL terminated array of paths.
 * role is "user", "assistant" or "system". */
spk_status spk_session_append(spk_session * session, const char * role,
                              const char * text, const char * const * images);

/* evaluate the system prompt once and snapshot its KV state, so the next turns
 * skip that prefill. Returns SPK_ERR_UNSUPPORTED when the model has no memory. */
spk_status spk_session_warmup(spk_session * session);

/* run one generation over the accumulated turns. the last appended turn must be
 * the user turn. Blocks until finished, cancelled, or max_tokens is reached. */
spk_status spk_session_generate(spk_session * session, const spk_gen_params * params,
                                spk_token_cb on_token, void * user_data);

int32_t spk_session_n_past(const spk_session * session);
int32_t spk_session_n_ctx(const spk_session * session);

/* raw checkpoint of the whole sequence (not just the system prefix) */
spk_status spk_session_checkpoint(spk_session * session);
spk_status spk_session_restore(spk_session * session);

/* ------------------------------------------------------------------ */
/* capabilities                                                        */
/* ------------------------------------------------------------------ */

bool spk_supports_vision(const spk_engine * engine);
bool spk_supports_translate(const spk_engine * engine);
bool spk_supports_asr(const spk_engine * engine);

/* one shot caption / VQA on an image file, streams the answer */
spk_status spk_describe_image(spk_engine * engine, const char * image_path,
                              const char * question, const spk_gen_params * params,
                              spk_token_cb on_token, void * user_data);

/* dedicated translation model if attached, otherwise the main SLM is prompted */
spk_status spk_translate(spk_engine * engine, const char * text,
                         const char * src_lang, const char * tgt_lang,
                         const spk_gen_params * params,
                         spk_token_cb on_token, void * user_data);

/* host ASR. When no whisper model is attached the callback is used instead. */
void       spk_asr_set_host_callback(spk_engine * engine, spk_host_asr_cb cb, void * user_data);
spk_status spk_asr_transcribe(spk_engine * engine, const char * audio_path,
                              const char * language, char * out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* SPK_H */
