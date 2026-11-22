/*
 * spk_core.h - the ONLY translation unit of this kernel that is allowed to
 * mention the upstream runtime's identifiers.
 *
 * Everything else in kernel/src speaks spk_* only. This keeps the Siri Plus
 * Kernel brand-clean at the source level while still honouring the MIT
 * attribution of the runtime it is built on (see third_party/NOTICE.md).
 *
 * If you are adding a new capability: add an alias here, never include the
 * upstream headers from another file.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "llama.h"
#include "mtmd.h"
#include "mtmd-helper.h"

// ------------------------------------------------------------- types
typedef llama_model                  spk_model;
typedef llama_context                spk_context;
typedef llama_vocab                  spk_vocab;
typedef llama_batch                  spk_batch;
typedef llama_sampler                spk_sampler;
typedef llama_memory_t               spk_memory;
typedef llama_token                  spk_token_id;
typedef llama_pos                    spk_pos;
typedef llama_context_params         spk_context_params;
typedef llama_model_params           spk_model_params;
typedef llama_sampler_chain_params   spk_sampler_params;
typedef llama_chat_message           spk_chat_message;
typedef ggml_log_level               spk_log_level;
typedef mtmd_context_params          spk_vision_params;
typedef llama_seq_id                 spk_seq_id;

typedef mtmd_context                 spk_vision_ctx;
typedef mtmd_bitmap                  spk_bitmap;
typedef mtmd_input_chunks            spk_input_chunks;
typedef mtmd_input_text              spk_input_text;
typedef mtmd_helper_bitmap_wrapper   spk_bitmap_wrapper;
typedef mtmd_helper_init_opt         spk_vision_opt;

// ------------------------------------------------------------- enums
#define SPK_TYPE_Q8                GGML_TYPE_Q8_0
#define SPK_TYPE_F16               GGML_TYPE_F16
#define SPK_FLASH_AUTO             LLAMA_FLASH_ATTN_TYPE_AUTO
#define SPK_FLASH_DISABLED         LLAMA_FLASH_ATTN_TYPE_DISABLED
#define SPK_LOAD_MMAP              LLAMA_LOAD_MODE_MMAP
#define SPK_LOAD_NONE              LLAMA_LOAD_MODE_NONE
#define SPK_DEFAULT_SEED           LLAMA_DEFAULT_SEED
#define SPK_LOG_LEVEL_INFO         GGML_LOG_LEVEL_INFO

// ------------------------------------------------------------ model
#define spk_backend_init                 llama_backend_init
#define spk_supports_gpu_offload         llama_supports_gpu_offload
#define spk_model_default_params         llama_model_default_params
#define spk_model_load                   llama_model_load_from_file
#define spk_model_free                   llama_model_free
#define spk_model_size                   llama_model_size
#define spk_model_n_params               llama_model_n_params
#define spk_model_desc                   llama_model_desc
#define spk_model_n_ctx_train            llama_model_n_ctx_train
#define spk_model_n_layer                llama_model_n_layer
#define spk_model_n_embd                 llama_model_n_embd
#define spk_model_is_recurrent           llama_model_is_recurrent
#define spk_model_has_encoder            llama_model_has_encoder
#define spk_model_has_decoder            llama_model_has_decoder
#define spk_model_chat_template          llama_model_chat_template
#define spk_model_get_vocab              llama_model_get_vocab
#define spk_model_decoder_start_token    llama_model_decoder_start_token

// ---------------------------------------------------------- context
#define spk_context_default_params       llama_context_default_params
#define spk_init_from_model              llama_init_from_model
#define spk_free                         llama_free
#define spk_n_ctx                        llama_n_ctx
#define spk_get_memory                   llama_get_memory
#define spk_memory_clear                 llama_memory_clear
#define spk_get_logits_ith               llama_get_logits_ith

// ------------------------------------------------------------- state
#define spk_state_size                   llama_state_seq_get_size
#define spk_state_read                   llama_state_seq_get_data
#define spk_state_write                  llama_state_seq_set_data

// --------------------------------------------------------- inference
#define spk_batch_init                   llama_batch_init
#define spk_batch_free                   llama_batch_free
#define spk_decode                       llama_decode
#define spk_encode                       llama_encode

// ----------------------------------------------------------- vocab
#define spk_vocab_is_eog                 llama_vocab_is_eog
#define spk_vocab_n_tokens               llama_vocab_n_tokens
#define spk_tokenize                     llama_tokenize
#define spk_token_to_piece               llama_token_to_piece
#define spk_chat_apply_template          llama_chat_apply_template

// --------------------------------------------------------- samplers
#define spk_sampler_params_default       llama_sampler_chain_default_params
#define spk_sampler_chain                llama_sampler_chain_init
#define spk_sampler_add                  llama_sampler_chain_add
#define spk_sampler_top_k                llama_sampler_init_top_k
#define spk_sampler_top_p                llama_sampler_init_top_p
#define spk_sampler_min_p                llama_sampler_init_min_p
#define spk_sampler_temp                 llama_sampler_init_temp
#define spk_sampler_penalties            llama_sampler_init_penalties
#define spk_sampler_dist                 llama_sampler_init_dist
#define spk_sampler_greedy               llama_sampler_init_greedy
#define spk_sampler_sample               llama_sampler_sample
#define spk_sampler_accept               llama_sampler_accept
#define spk_sampler_reset                llama_sampler_reset
#define spk_sampler_free                 llama_sampler_free

// -------------------------------------------------------------- sys
#define spk_log_set                      llama_log_set
#define spk_time_us                      llama_time_us

// ------------------------------------------------------------ vision
#define spk_vision_params_default        mtmd_context_params_default
#define spk_vision_init                  mtmd_init_from_file
#define spk_vision_free                  mtmd_free
#define spk_vision_support               mtmd_support_vision
#define spk_vision_marker                mtmd_default_marker
#define spk_vision_chunks_init           mtmd_input_chunks_init
#define spk_vision_chunks_free           mtmd_input_chunks_free
#define spk_vision_tokenize              mtmd_tokenize
#define spk_vision_helper_eval           mtmd_helper_eval_chunks
#define spk_vision_bitmap_from_file      mtmd_helper_bitmap_init_from_file
#define spk_vision_bitmap_free           mtmd_bitmap_free
#define spk_vision_opt_default           mtmd_helper_init_opt_default
