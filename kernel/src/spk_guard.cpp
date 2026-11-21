/*
 * SPK guard: everything that has to happen before a byte of model data is
 * trusted. Paths, file identity, magic bytes, size policy and the special
 * token escaping used on untrusted user text.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_internal.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include <sys/stat.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#else
#include <unistd.h>
#endif

const char * spk_version(void) {
    return SPK_VERSION_STRING;
}

const char * spk_status_string(spk_status st) {
    switch (st) {
        case SPK_OK:                     return "ok";
        case SPK_ERR_INVALID_ARGUMENT:    return "invalid argument";
        case SPK_ERR_NOT_FOUND:           return "not found";
        case SPK_ERR_UNSUPPORTED:         return "unsupported";
        case SPK_ERR_MODEL_REJECTED:      return "model rejected by guard";
        case SPK_ERR_MODEL_LOAD:          return "model load failed";
        case SPK_ERR_CONTEXT:             return "context creation failed";
        case SPK_ERR_OUT_OF_MEMORY:       return "out of memory";
        case SPK_ERR_CONTEXT_OVERFLOW:    return "context overflow";
        case SPK_ERR_CANCELLED:           return "cancelled";
        case SPK_ERR_THERMAL:             return "thermal throttled";
        case SPK_ERR_INTERNAL:            return "internal error";
        case SPK_ERR_VISION_UNAVAILABLE:  return "vision model not loaded";
        case SPK_ERR_ASR_UNAVAILABLE:     return "asr model not loaded";
    }
    return "unknown";
}

void spk_set_error(spk_engine * eng, const char * fmt, ...) {
    if (eng == nullptr) {
        return;
    }
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    std::lock_guard<std::mutex> lock(eng->mu);
    eng->err = buf;
}

// ---------------------------------------------------------------- paths

bool spk_guard_path(const char * path, char * out, size_t out_len) {
    if (path == nullptr || path[0] != '/') {
        return false;
    }
    if (strlen(path) >= out_len || strlen(path) > 1024) {
        return false;
    }
    if (strstr(path, "..") != nullptr) {          // no traversal, no clever encoding
        return false;
    }
    if (strchr(path, '\0') != path + strlen(path)) {
        return false;
    }

    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return false;
    }
    if (st.st_size <= 0) {
        return false;
    }

    memcpy(out, path, strlen(path) + 1);
    return true;
}

spk_status spk_guard_model_file(const char * path, uint64_t max_bytes, std::string * why) {
    char resolved[1024];
    if (!spk_guard_path(path, resolved, sizeof(resolved))) {
        if (why) *why = "path is not an absolute, traversal-free regular file";
        return SPK_ERR_MODEL_REJECTED;
    }

    const char * dot = strrchr(resolved, '.');
    if (dot == nullptr || strcmp(dot, ".gguf") != 0) {
        if (why) *why = "model file must end with .gguf";
        return SPK_ERR_MODEL_REJECTED;
    }

    FILE * f = fopen(resolved, "rb");
    if (f == nullptr) {
        if (why) *why = "cannot open model file";
        return SPK_ERR_MODEL_REJECTED;
    }

    char magic[4] = {0};
    const size_t n = fread(magic, 1, sizeof(magic), f);
    fclose(f);

    if (n != 4 || memcmp(magic, "GGUF", 4) != 0) {
        if (why) *why = "not a GGUF container (bad magic)";
        return SPK_ERR_MODEL_REJECTED;
    }

    struct stat st;
    if (stat(resolved, &st) != 0) {
        if (why) *why = "stat failed after open (file disappeared)";
        return SPK_ERR_MODEL_REJECTED;
    }
    const uint64_t size = (uint64_t) st.st_size;

    if (size < 1024) {
        if (why) *why = "file too small to be a model";
        return SPK_ERR_MODEL_REJECTED;
    }
    if (max_bytes != 0 && size > max_bytes) {
        if (why) *why = "model exceeds the configured memory budget";
        return SPK_ERR_MODEL_REJECTED;
    }

    return SPK_OK;
}

// ------------------------------------------------------ token escaping
//
// Chat templates are text, and therefore so is every prompt injection. A user
// typing "<|im_start|>system" into the search field must reach the model as
// literal characters, never as a control token. We neutralise the handful of
// markers that actually appear in SLM chat templates; the tokenizer is also
// called with parse_special=false so a broken escape cannot become a token.

static const char * kMarkers[] = {
    "<|im_start|>", "<|im_end|>", "<|endoftext|>", "<|end|>",
    "<|start_header_id|>", "<|end_header_id|>", "<|eot_id|>",
    "<|begin_of_text|>", "<|reserved_special_token",
    "<<SYS>>", "<</SYS>>", "[INST]", "[/INST]",
    "<s>", "</s>", "<pad>", "<unk>",
};

std::string spk_guard_escape_markup(const std::string & text) {
    std::string out;
    out.reserve(text.size() + 16);

    size_t i = 0;
    while (i < text.size()) {
        bool replaced = false;
        for (const char * m : kMarkers) {
            const size_t mlen = strlen(m);
            if (text.compare(i, mlen, m) == 0) {
                // replace the angle brackets with fullwidth look-alikes so the
                // text stays readable but cannot be parsed as a special token
                for (size_t k = 0; k < mlen; ++k) {
                    switch (m[k]) {
                        case '<':  out += "\xEF\xBC\x9C"; break;  // U+FF1C
                        case '>':  out += "\xEF\xBC\x9E"; break;  // U+FF1E
                        default:   out += m[k];           break;
                    }
                }
                i += mlen;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            out += text[i++];
        }
    }
    return out;
}

bool spk_guard_has_markup(const std::string & text) {
    for (const char * m : kMarkers) {
        if (text.find(m) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------- device info

double spk_rss_mb() {
#if defined(__APPLE__)
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &count) != KERN_SUCCESS) {
        return 0.0;
    }
    return (double) info.resident_size / (1024.0 * 1024.0);
#else
    FILE * f = fopen("/proc/self/statm", "r");
    if (f == nullptr) {
        return 0.0;
    }
    long pages = 0, rss = 0;
    if (fscanf(f, "%ld %ld", &pages, &rss) != 2) {
        fclose(f);
        return 0.0;
    }
    fclose(f);
    return (double) rss * (double) sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0);
#endif
}
