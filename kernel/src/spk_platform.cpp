/*
 * LLAI Kernel - platform probes
 *
 * Only this file knows how a given OS reports RAM, cores and heat. Everything
 * that makes decisions (context size, batch size, thread count, thermal policy)
 * asks the ops table instead, which is what keeps the kernel portable.
 *
 * SPDX-License-Identifier: MIT
 */
#include "spk_platform.h"

#include <thread>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <unistd.h>
#endif

namespace {

uint64_t default_ram(void * /*ud*/) {
#if defined(_WIN32)
    MEMORYSTATUSEX st;
    st.dwLength = sizeof(st);
    if (!GlobalMemoryStatusEx(&st)) {
        return 0;
    }
    return (uint64_t) st.ullTotalPhys;
#elif defined(__APPLE__)
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

int32_t default_cores(void * /*ud*/) {
#if defined(_WIN32)
    // Groups larger than 64 cores need the explicit call; on a laptop it is the
    // physical core count, which is what decoding should use.
    const DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (n > 0) {
        return (int32_t) n;
    }
#endif
    const int32_t hw = (int32_t) std::thread::hardware_concurrency();
    return hw > 0 ? hw : 2;
}

int32_t default_thermal(void * /*ud*/) {
    // ProcessInfo is the only thermal source iOS exposes; the bridge pushes the
    // level in through spk_engine_set_thermal_state, so nothing to read here.
    return -1;
}

const spk_platform_ops kDefaultOps = {
    default_ram,
    default_cores,
    default_thermal,
    nullptr,
};

spk_platform_ops g_ops = kDefaultOps;

} // namespace

extern "C" void spk_platform_set_ops(const spk_platform_ops * ops) {
    g_ops = (ops != nullptr) ? *ops : kDefaultOps;
}

extern "C" const spk_platform_ops * spk_platform_get_ops(void) {
    return &g_ops;
}
