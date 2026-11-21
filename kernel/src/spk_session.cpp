/*
 * SPK session: chat templating, prefix reuse, the decode loop and state
 * checkpointing. This is where the latency actually comes from.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

namespace {

double now_ms() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::milli>(clock::now().time_since_epoch()).count();
}

void spk_secure_zero(void * p, size_t n) {
    volatile uint8_t * v = (volatile uint8_t *) p;
    while (n--) {
        *v++ = 0;
    }
}

bool spk_eval_chunk(spk_engine * eng, spk_session * ses,
                    const std::vector<spk_token_id> & tokens,
                    size_t begin, size_t end, bool logits_last) {
    if (begin >= end) {
        return true;
    }
    const int32_t n = (int32_t) (end - begin);
    spk_batch batch = spk_batch_init(n, 0, 1);
    for (int32_t i = 0; i < n; ++i) {
        batch.token[i]    = tokens[begin + (size_t) i];
        batch.pos[i]      = ses->n_past + i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i]   = (logits_last && i == n - 1) ? 1 : 0;
    }
    const int32_t rc = spk_decode(ses->eng->text.ctx, batch);
    spk_batch_free(batch);

    if (rc != 0) {
        spk_set_error(eng, "decode failed with %d at pos %d", rc, (int) ses->n_past);
        return false;
    }
    ses->n_past += n;
    return true;
}

// prefill in n_batch sized slabs so a 4096 token prompt does not allocate one
// enormous graph on a 4 GB device
bool spk_prefill(spk_engine * eng, spk_session * ses,
                 const std::vector<spk_token_id> & tokens, size_t from, bool logits_last) {
    const int32_t nb = (int32_t) std::max(64, eng->cfg.n_batch > 0 ? eng->cfg.n_batch : 256);
    size_t cursor = from;
    while (cursor < tokens.size()) {
        if (eng->cancel.load()) {
            return false;
        }
        const size_t end = std::min(tokens.size(), cursor + (size_t) nb);
        const bool last = (end == tokens.size());
        if (!spk_eval_chunk(eng, ses, tokens, cursor, end, logits_last && last)) {
            return false;
        }
        cursor = end;
    }
    return true;
}

int32_t spk_tokenize_text(spk_engine * eng, const std::string & text,
                          std::vector<spk_token_id> * out) {
    const spk_vocab * vocab = spk_model_get_vocab(eng->text.model);
    const int32_t n_max = (int32_t) std::min<size_t>(text.size() + 16, 65536);

    std::vector<spk_token_id> buf((size_t) n_max);
    int32_t n = spk_tokenize(vocab, text.c_str(), (int32_t) text.size(),
                             buf.data(), n_max, true, true);
    if (n < 0) {
        // negative means "would have needed this many"; grow once and retry
        const int32_t need = -n;
        if (need <= 0 || need > 65536) {
            return -1;
        }
        buf.resize((size_t) need);
        n = spk_tokenize(vocab, text.c_str(), (int32_t) text.size(),
                         buf.data(), need, true, true);
        if (n < 0) {
            return -1;
        }
    }
    buf.resize((size_t) n);
    *out = std::move(buf);
    return n;
}

} // namespace

// ------------------------------------------------------- template

// Qwen 3.x / 3.5 ship a ~7.8 KB jinja template: tool-call branches,
// raise_exception() on bad message shapes, and an enable_thinking switch.
// No matcher in this kernel evaluates jinja, so the Qwen wire format is
// rendered directly. The generation prompt carries the same empty reasoning
// block the template emits when enable_thinking is off, which keeps the first
// token at a few hundred milliseconds instead of a 200-800 token monologue.
// A voice assistant that answers in 663 ms beats one that is smarter late.
namespace {

bool spk_arch_is_qwen_chat(const std::string & arch) {
    return arch.rfind("qwen", 0) == 0;
}

std::string spk_chatml_manual(const std::vector<spk_msg> & msgs,
                              bool add_ass, bool no_think) {
    std::string out;
    out.reserve(512);
    for (const auto & m : msgs) {
        out += "<|im_start|>";
        out += m.role;
        out += "\n";
        out += m.text;
        out += "<|im_end|>\n";
    }
    if (add_ass) {
        out += "<|im_start|>assistant\n";
        if (no_think) {
            out += "<think>\n\n</think>\n\n";
        }
    }
    return out;
}

} // namespace

std::string spk_apply_template(spk_engine * eng, const std::vector<spk_msg> & msgs, bool add_ass) {
    if (eng == nullptr || eng->text.model == nullptr || msgs.empty()) {
        return std::string();
    }

    // Fast path for the bundled Qwen family: render its wire format here.
    // The user supplied chat_template (if any) still wins, because that is an
    // explicit override from the host app.
    if (eng->chat_template.empty() && spk_arch_is_qwen_chat(spk_engine_arch(eng))) {
        return spk_chatml_manual(msgs, add_ass, true);
    }

    const char * tmpl = !eng->chat_template.empty()
        ? eng->chat_template.c_str()
        : spk_model_chat_template(eng->text.model, nullptr);
    if (tmpl == nullptr) {
        // every SLM worth running ships a template; chatml is the sane default
        tmpl = "chatml";
    }

    std::vector<spk_chat_message> chat;
    chat.reserve(msgs.size());
    for (const auto & m : msgs) {
        chat.push_back({ m.role.c_str(), m.text.c_str() });
    }

    size_t guess = 512;
    for (const auto & m : msgs) {
        guess += m.text.size() * 2;
    }

    std::vector<char> buf(guess);
    int32_t n = spk_chat_apply_template(tmpl, chat.data(), chat.size(), add_ass,
                                        buf.data(), (int32_t) buf.size());
    if (n < 0) {
        // keep the diagnostic, but never drop the turn on the floor
        spk_set_error(eng, "chat template fallback to chatml (%d)", n);
        return spk_chatml_manual(msgs, add_ass, false);
    }
    if ((size_t) n > buf.size()) {
        buf.resize((size_t) n);
        n = spk_chat_apply_template(tmpl, chat.data(), chat.size(), add_ass,
                                    buf.data(), (int32_t) buf.size());
        if (n < 0 || (size_t) n > buf.size()) {
            spk_set_error(eng, "chat template output does not fit, using chatml");
            return spk_chatml_manual(msgs, add_ass, false);
        }
    }
    return std::string(buf.data(), (size_t) n);
}

// ------------------------------------------------------- sessions

spk_status spk_session_create(spk_engine * eng, spk_session ** out) {
    if (eng == nullptr || out == nullptr || eng->text.model == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    auto * ses = new spk_session();
    ses->eng    = eng;
    ses->system = eng->system_prompt.empty()
        ? "You are Siri AI, a concise on-device assistant. Answer in the user's language."
        : eng->system_prompt;
    *out = ses;
    return SPK_OK;
}

void spk_session_destroy(spk_session * ses) {
    if (ses == nullptr) {
        return;
    }
    if (ses->eng != nullptr && ses->eng->cfg.secure_mode) {
        spk_secure_zero(ses->prefix.data(), ses->prefix.size());
        spk_secure_zero(ses->checkpoint.data(), ses->checkpoint.size());
    }
    if (ses->smpl != nullptr) {
        spk_sampler_free(ses->smpl);
        ses->smpl = nullptr;
    }
    delete ses;
}

spk_status spk_session_reset(spk_session * ses) {
    if (ses == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);
    ses->msgs.clear();
    ses->tokens.clear();
    ses->n_past = 0;
    ses->checkpoint.clear();
    ses->checkpoint_len = 0;
    // keep the prefix snapshot: the system prompt did not change
    return SPK_OK;
}

spk_status spk_session_set_system(spk_session * ses, const char * system_prompt) {
    if (ses == nullptr || system_prompt == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);
    ses->system = system_prompt;
    ses->prefix.clear();
    ses->prefix_tokens.clear();
    ses->tokens.clear();
    ses->n_past = 0;
    return SPK_OK;
}

spk_status spk_session_append(spk_session * ses, const char * role,
                              const char * text, const char * const * images) {
    if (ses == nullptr || role == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::string r(role);
    if (r != "user" && r != "assistant" && r != "system") {
        return SPK_ERR_INVALID_ARGUMENT;
    }

    std::string body = text != nullptr ? text : "";
    if (ses->eng->cfg.secure_mode) {
        body = spk_guard_escape_markup(body);
    }

    std::lock_guard<std::mutex> lock(ses->mu);
    spk_msg m;
    m.role = r;
    m.text = body;
    if (images != nullptr) {
        for (size_t i = 0; images[i] != nullptr; ++i) {
            char resolved[1024];
            if (spk_guard_path(images[i], resolved, sizeof(resolved))) {
                m.images.emplace_back(resolved);
            } else {
                spk_set_error(ses->eng, "image path rejected: %s", images[i]);
                return SPK_ERR_INVALID_ARGUMENT;
            }
        }
    }
    ses->msgs.push_back(std::move(m));
    if (r == "user") {
        ses->checkpoint.clear();
        ses->checkpoint_len = 0;
    }
    return SPK_OK;
}

spk_status spk_session_warmup(spk_session * ses) {
    if (ses == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);
    spk_engine * eng = ses->eng;

    std::vector<spk_msg> only_system;
    only_system.push_back({"system", ses->system});
    const std::string prompt = spk_apply_template(eng, only_system, false);
    if (prompt.empty()) {
        return SPK_ERR_INTERNAL;
    }

    std::vector<spk_token_id> toks;
    if (spk_tokenize_text(eng, prompt, &toks) <= 0) {
        spk_set_error(eng, "system prompt did not tokenize");
        return SPK_ERR_INTERNAL;
    }

    spk_memory_clear(spk_get_memory(eng->text.ctx), false);
    ses->n_past = 0;
    if (!spk_prefill(eng, ses, toks, 0, true)) {
        return SPK_ERR_CONTEXT;
    }

    const size_t need = spk_state_size(eng->text.ctx, 0);
    if (need == 0) {
        return SPK_ERR_UNSUPPORTED;
    }
    ses->prefix.assign(need, 0);
    if (spk_state_read(eng->text.ctx, ses->prefix.data(), ses->prefix.size(), 0) != need) {
        ses->prefix.clear();
        return SPK_ERR_INTERNAL;
    }
    ses->prefix_tokens = toks;
    return SPK_OK;
}

// One sampling chain per session, keyed on the generation parameters. seed == 0
// means "random for this chain", so the reuse keeps the RNG advancing instead of
// reseeding on every turn.
static spk_sampler * spk_session_sampler(spk_session * ses,
                                         const spk_gen_params * params,
                                         int32_t n_vocab) {
    spk_gen_params p{};
    if (params != nullptr) {
        p = *params;
    }
    const bool reusable = ses->smpl != nullptr &&
        ses->smpl_vocab == n_vocab &&
        std::memcmp(&ses->smpl_cfg, &p, sizeof(p)) == 0;
    if (reusable) {
        return ses->smpl;
    }
    if (ses->smpl != nullptr) {
        spk_sampler_free(ses->smpl);
        ses->smpl = nullptr;
    }
    ses->smpl = spk_make_sampler(params, n_vocab, spk_engine_arch(ses->eng).c_str());
    if (ses->smpl != nullptr) {
        ses->smpl_cfg   = p;
        ses->smpl_vocab = n_vocab;
    }
    return ses->smpl;
}

spk_status spk_session_generate(spk_session * ses, const spk_gen_params * params,
                                spk_token_cb on_token, void * user_data) {
    if (ses == nullptr || ses->eng == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);

    spk_engine * eng = ses->eng;
    if (eng->text.ctx == nullptr) {
        return SPK_ERR_CONTEXT;
    }
    if (ses->msgs.empty()) {
        spk_set_error(eng, "nothing to answer");
        return SPK_ERR_INVALID_ARGUMENT;
    }

    eng->cancel.store(false);
    const double t_start = now_ms();

    // ---- build the prompt -------------------------------------------
    std::vector<spk_msg> msgs;
    if (!ses->system.empty()) {
        msgs.push_back({"system", ses->system});
    }
    for (const auto & m : ses->msgs) {
        msgs.push_back(m);
    }

    bool use_vision = false;
    if (!ses->msgs.back().images.empty()) {
        if (eng->vision.vision == nullptr) {
            spk_set_error(eng, "an image was attached but no vision model is loaded");
            return SPK_ERR_VISION_UNAVAILABLE;
        }
        use_vision = true;
    }

    spk_pos     n_past     = 0;
    spk_pos     n_prefilled = 0;
    std::string prompt;

    if (use_vision) {
        // put the media marker in front of the question
        const char * marker = spk_vision_marker();
        spk_msg & last = msgs.back();
        last.text = std::string(marker) + "\n" + last.text;

        prompt = spk_apply_template(eng, msgs, true);
        if (prompt.empty()) {
            return SPK_ERR_INTERNAL;
        }
        spk_memory_clear(spk_get_memory(eng->text.ctx), false);
        ses->n_past = 0;
        ses->tokens.clear();

        const double tv0 = now_ms();
        spk_status st = spk_vision_eval(eng, ses, prompt, ses->msgs.back().images, 0, true, &n_past);
        if (st != SPK_OK) {
            return st;
        }
        n_prefilled = n_past;
        ses->last.prefill_tok_s = 0.0;
        eng->metrics.ttft_ms = std::fmax(0.0, now_ms() - tv0);
    } else {
        prompt = spk_apply_template(eng, msgs, true);
        if (prompt.empty()) {
            return SPK_ERR_INTERNAL;
        }

        std::vector<spk_token_id> toks;
        if (spk_tokenize_text(eng, prompt, &toks) <= 0) {
            spk_set_error(eng, "prompt did not tokenize");
            return SPK_ERR_INTERNAL;
        }

        const int32_t n_ctx = (int32_t) spk_n_ctx(eng->text.ctx);
        if ((int32_t) toks.size() >= n_ctx - 8) {
            spk_set_error(eng, "prompt has %zu tokens, context is %d", toks.size(), n_ctx);
            return SPK_ERR_CONTEXT_OVERFLOW;
        }

        // ---- prefix reuse -------------------------------------------
        size_t from = 0;
        if (!ses->prefix.empty() && !ses->prefix_tokens.empty() &&
            toks.size() >= ses->prefix_tokens.size() &&
            memcmp(toks.data(), ses->prefix_tokens.data(),
                   ses->prefix_tokens.size() * sizeof(spk_token_id)) == 0) {
            const spk_pos plen = (spk_pos) ses->prefix_tokens.size();
            if (spk_state_write(eng->text.ctx, ses->prefix.data(), ses->prefix.size(), 0) ==
                ses->prefix.size()) {
                n_past = plen;
                from   = ses->prefix_tokens.size();
                ses->tokens = toks;
                ses->n_past = plen;
            }
        }

        if (n_past == 0) {
            spk_memory_clear(spk_get_memory(eng->text.ctx), false);
            ses->tokens.clear();
            ses->n_past = 0;
        }

        const double tp0 = now_ms();
        if (!spk_prefill(eng, ses, toks, from, true)) {
            return eng->cancel.load() ? SPK_ERR_CANCELLED : SPK_ERR_CONTEXT;
        }
        ses->tokens = toks;
        n_past = ses->n_past;
        n_prefilled = n_past - (spk_pos) from;

        const double dt = std::fmax(1e-3, now_ms() - tp0);
        ses->last.prefill_tok_s = (double) n_prefilled * 1000.0 / dt;
    }

    // ---- decode -------------------------------------------------------
    const spk_vocab * vocab = spk_model_get_vocab(eng->text.model);
    spk_sampler * smpl = spk_session_sampler(ses, params, spk_vocab_n_tokens(vocab));
    if (smpl == nullptr) {
        return SPK_ERR_INTERNAL;
    }
    spk_sampler_reset(smpl);
    const int32_t n_ctx = (int32_t) spk_n_ctx(eng->text.ctx);
    const int32_t max_tokens = (params != nullptr && params->max_tokens > 0)
        ? params->max_tokens : 256;

    int64_t generated = 0;
    bool first = true;
    spk_status status = SPK_OK;
    double t_decode = 0.0;

    // Reuse a single one-token batch: allocating one per token used to cost
    // half a dozen malloc/free pairs per emitted token, which shows up as
    // timing jitter on an otherwise smooth 30 tok/s decode.
    spk_batch one = spk_batch_init(1, 0, 1);
    if (one.token == nullptr) {
        return SPK_ERR_INTERNAL;
    }

    // Coalesce pieces before they cross the C/Swift boundary: one callback per
    // token wakes the bridge and the UI thread ~30 times a second, and re-layout
    // on every character is what makes old phones feel slow.
    std::string pending;
    auto last_flush = std::chrono::steady_clock::now();
    auto flush = [&](bool force) {
        if (pending.empty()) {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        const double age = std::chrono::duration<double, std::milli>(now - last_flush).count();
        if (!force && pending.size() < 96 && age < 20.0) {
            return;
        }
        spk_emit(on_token, user_data, pending.c_str(), false);
        pending.clear();
        last_flush = now;
    };

    for (int32_t i = 0; i < max_tokens; ++i) {
        if (eng->cancel.load()) {
            status = SPK_ERR_CANCELLED;
            break;
        }
        if (ses->n_past + 1 >= n_ctx) {
            status = SPK_ERR_CONTEXT_OVERFLOW;
            break;
        }

        spk_token_id id = spk_sampler_sample(smpl, eng->text.ctx, -1);
        if (spk_vocab_is_eog(vocab, id)) {
            break;
        }

        char piece[256];
        int32_t np = spk_token_to_piece(vocab, id, piece, (int32_t) sizeof(piece) - 1, 0, false);
        if (np > 0) {
            pending.append(piece, (size_t) np);
            flush(false);
        }

        ses->tokens.push_back(id);
        spk_sampler_accept(smpl, id);
        generated++;

        if (first) {
            first = false;
            ses->last.ttft_ms = std::fmax(0.0, now_ms() - t_start);
        }

        // thermal pacing: give the scheduler a hint instead of letting iOS
        // throttle us mid-graph
        if (eng->thermal.load() >= SPK_THERMAL_SERIOUS && (i % 8) == 7) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        const double td0 = now_ms();
        one.token[0]    = id;
        one.pos[0]      = ses->n_past;
        one.n_seq_id[0] = 1;
        one.seq_id[0][0] = 0;
        one.logits[0]   = 1;
        const int32_t rc = spk_decode(eng->text.ctx, one);
        if (rc != 0) {
            spk_set_error(eng, "decode failed with %d", rc);
            status = SPK_ERR_CONTEXT;
            break;
        }
        ses->n_past += 1;
        t_decode += now_ms() - td0;
    }

    flush(true);
    spk_batch_free(one);
    spk_emit(on_token, user_data, nullptr, true);

    if (generated > 1 && t_decode > 0.0) {
        ses->last.decode_tok_s = (double) (generated - 1) * 1000.0 / t_decode;
    }
    ses->last.n_prefill_tokens = n_prefilled;
    ses->last.n_decode_tokens  = generated;
    ses->last.peak_rss_mb      = std::fmax(ses->last.peak_rss_mb, spk_rss_mb());

    {
        std::lock_guard<std::mutex> glock(eng->mu);
        eng->metrics = ses->last;
    }
    return status;
}

int32_t spk_session_n_past(const spk_session * ses) {
    return ses != nullptr ? (int32_t) ses->n_past : 0;
}

int32_t spk_session_n_ctx(const spk_session * ses) {
    return (ses != nullptr && ses->eng != nullptr && ses->eng->text.ctx != nullptr)
        ? (int32_t) spk_n_ctx(ses->eng->text.ctx) : 0;
}

spk_status spk_session_checkpoint(spk_session * ses) {
    if (ses == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);
    spk_engine * eng = ses->eng;
    const size_t need = spk_state_size(eng->text.ctx, 0);
    if (need == 0) {
        return SPK_ERR_UNSUPPORTED;
    }
    ses->checkpoint.assign(need, 0);
    if (spk_state_read(eng->text.ctx, ses->checkpoint.data(), ses->checkpoint.size(), 0) != need) {
        ses->checkpoint.clear();
        return SPK_ERR_INTERNAL;
    }
    ses->checkpoint_len = ses->n_past;
    return SPK_OK;
}

spk_status spk_session_restore(spk_session * ses) {
    if (ses == nullptr) {
        return SPK_ERR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(ses->mu);
    if (ses->checkpoint.empty()) {
        return SPK_ERR_NOT_FOUND;
    }
    if (spk_state_write(ses->eng->text.ctx, ses->checkpoint.data(),
                        ses->checkpoint.size(), 0) != ses->checkpoint.size()) {
        return SPK_ERR_INTERNAL;
    }
    ses->n_past = ses->checkpoint_len;
    ses->tokens.resize((size_t) ses->checkpoint_len > ses->tokens.size()
                       ? ses->tokens.size() : (size_t) ses->checkpoint_len);
    return SPK_OK;
}
