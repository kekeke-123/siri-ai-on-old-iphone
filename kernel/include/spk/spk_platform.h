/*
 * LLAI Kernel - platform layer
 *
 * The kernel is portable C++17. It never includes Apple frameworks and never
 * reads device state on its own: RAM, core counts and thermal state arrive
 * through this tiny ops table. On iOS the bridge installs the default table
 * (sysctl); an Android, Linux, Windows or simulator host overrides it and the
 * scheduling policy adapts without touching the inference code.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SPK_PLATFORM_H
#define SPK_PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spk_platform_ops {
    /* total physical RAM in bytes, 0 when unknown */
    uint64_t (*physical_ram_bytes)(void * user_data);
    /* cores worth scheduling decoding onto, 0 = let the kernel guess */
    int32_t  (*performance_cores)(void * user_data);
    /* 0 nominal, 1 fair, 2 serious, 3 critical; -1 when the host has no sensor */
    int32_t  (*thermal_level)(void * user_data);
    void * user_data;
} spk_platform_ops;

/* Install a host table. Passing NULL restores the compiled-in defaults, which
 * use sysctl on Apple platforms and sysconf elsewhere. The table is copied. */
void spk_platform_set_ops(const spk_platform_ops * ops);

/* Current table, never NULL. */
const spk_platform_ops * spk_platform_get_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* SPK_PLATFORM_H */
