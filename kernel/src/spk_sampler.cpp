/*
 * SPK sampler chain. One place to decide how a token is chosen, so the app,
 * the translation path and the vision path all sample the same way.
 *
 * Defaults are arch aware: the Qwen family (2.5 / 3 / 3.5, including the
 * hybrid qwen35 and qwen35moe builds) is tuned at temperature 0.7,
 * top_p 0.8, top_k 20, repeat 1.05 by its own model cards. Running the old
 * generic 40 / 0.95 / 1.1 set on a 0.8B model produces noticeably loopier
 * answers, so the constants live here instead of in every caller.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <cmath>
#include <cstring>

static bool spk_arch_is_qwen(const char * arch) {
    // "qwen2", "qwen3", "qwen35", "qwen35moe", "qwen3next" all start with qwen
    return arch != nullptr && std::strncmp(arch, "qwen", 4) == 0;
}

spk_sampler * spk_make_sampler(const spk_gen_params * in, int32_t n_vocab, const char * arch) {
    spk_gen_params p{};
    if (in != nullptr) {
        p = *in;
    }

    const bool qwen = spk_arch_is_qwen(arch);

    if (p.max_tokens <= 0)      p.max_tokens      = 256;
    if (p.top_k <= 0)           p.top_k           = qwen ? 20 : 40;
    if (p.top_p <= 0.0f)        p.top_p           = qwen ? 0.8f : 0.95f;
    if (p.min_p < 0.0f)         p.min_p           = 0.0f;
    if (p.temperature <= 0.0f)  p.temperature     = 0.7f;
    if (p.repeat_penalty <= 0)  p.repeat_penalty  = qwen ? 1.05f : 1.1f;
    if (p.repeat_last_n <= 0)   p.repeat_last_n   = 64;

    spk_sampler_params sp = spk_sampler_params_default();
    sp.no_perf = false;

    spk_sampler * chain = spk_sampler_chain(sp);

    // penalty first: it has to see the raw distribution
    spk_sampler_add(chain, spk_sampler_penalties(n_vocab,
                                                 p.repeat_last_n,
                                                 p.repeat_penalty,
                                                 0.0f,   // frequency penalty
                                                 0.0f)); // presence penalty

    if (p.temperature <= 0.0f) {
        spk_sampler_add(chain, spk_sampler_greedy());
        return chain;
    }

    if (p.top_k > 0) {
        spk_sampler_add(chain, spk_sampler_top_k(p.top_k));
    }
    if (p.top_p > 0.0f && p.top_p < 1.0f) {
        spk_sampler_add(chain, spk_sampler_top_p(p.top_p, 1));
    }
    if (p.min_p > 0.0f) {
        spk_sampler_add(chain, spk_sampler_min_p(p.min_p, 1));
    }
    spk_sampler_add(chain, spk_sampler_temp(p.temperature));

    const uint32_t seed = p.seed == 0 ? (uint32_t) spk_time_us() : (uint32_t) p.seed;
    spk_sampler_add(chain, spk_sampler_dist(seed));

    return chain;
}
