# LLAI 内核 · Windows x64

同一套 C++ 源码，和 iPhone 上跑的是同一份内核。Windows 端有两种用法：一个
独立的 `LLAI.exe`，和一组 `LLAI.dll` + `LLAI.lib` + 头文件。

## 1. exe：cmd 里直接调用

```bat
chcp 65001
LLAI.exe --model C:\models\Qwen3.5-0.8B-Q4_K_M.gguf --prompt "用一句话解释什么是本地推理" -n 128
```

第一次跑先 `chcp 65001`，否则中文会乱码（内核全程 UTF-8，控制台要跟着改）。

交互模式（不给 `--prompt` 就进 REPL）：

```bat
LLAI.exe -m C:\models\qwen.gguf
> 帮我写个正则，匹配 11 位手机号
> /reset
> /exit
```

管道也能用：

```bat
echo 你好 | LLAI.exe -m C:\models\qwen.gguf -n 64 --quiet
```

### 参数

| 参数 | 说明 |
|---|---|
| `-m, --model <path>` | GGUF 权重，必填，绝对路径 |
| `--mmproj <path>` | 视觉投影（可选） |
| `-p, --prompt <text>` | 一次性提问；省略则进交互 |
| `-n, --n <int>` | 最多生成 token，默认 256 |
| `--ctx <int>` | 上下文长度，默认按物理内存自动 |
| `-t, --threads <int>` | 解码线程，默认取物理核心数，上限 4 |
| `--temp <f>` | 温度，默认 0.7 |
| `--top-p <f>` | 核采样，默认 0.9 |
| `--system <text>` | 系统提示词 |
| `--no-warmup` | 跳过系统提示词预填 |
| `--quiet` | 关闭 stderr 上的统计行 |

统计行走 stderr（`[N tok | ttft 812 ms | 12.4 tok/s]`），答案走 stdout，
所以重定向很干净：`LLAI.exe -m q.gguf -p 你好 > answer.txt`。

## 2. dll：给别的程序调用

包里有 `LLAI.dll`、`LLAI.lib`、`spk.h`、`spk_platform.h`。C 接口，不需要
C++ ABI 兼容：

```c
#include "spk.h"
#include <stdio.h>

static void on_piece(const char * piece, bool final, void * ud) {
    fputs(piece, stdout);
    fflush(stdout);
}

int main(void) {
    spk_engine_config cfg = {0};
    cfg.model_path  = "C:\\models\\qwen.gguf";
    cfg.n_threads   = 0;      /* auto */
    cfg.n_ctx       = 0;      /* auto */
    cfg.use_metal   = false;  /* Windows: no GPU path yet */
    cfg.use_mmap    = true;
    cfg.secure_mode = false;

    char err[512] = {0};
    spk_engine * eng = NULL;
    if (spk_engine_create(&cfg, &eng, err, sizeof(err)) != SPK_OK) {
        printf("load failed: %s\n", err);
        return 1;
    }

    spk_session * ses = NULL;
    spk_session_create(eng, &ses);
    spk_session_warmup(ses);

    spk_gen_params gp = {0};
    gp.max_tokens = 200;
    spk_session_append(ses, "user", "你好，介绍一下你自己", NULL);
    spk_session_generate(ses, &gp, on_piece, NULL);
    printf("\n");

    spk_session_destroy(ses);
    spk_engine_destroy(eng);
    return 0;
}
```

编译（MSVC）：`cl sample.c /I. LLAI.lib /link /OUT:sample.exe`，或者运行时
加载：`LoadLibraryA("LLAI.dll")`。

`spk_` 前缀是稳定 ABI，产品名是 LLAI —— 这是刻意的：改名不破坏任何已发布的
调用点，下一轮才换前缀并同步发新 ABI。

## 3. 设备信息由宿主提供

内核不读 Apple 的设备信息。Windows 上默认平台表用 `GlobalMemoryStatusEx` 和
`GetActiveProcessorCount`；如果你的程序知道更好的数值，可以覆盖：

```c
static uint64_t my_ram(void * ud)   { return 4ull * 1024 * 1024 * 1024; }
static int32_t  my_cores(void * ud) { return 6; }
static int32_t  my_heat(void * ud)  { return 0; }

spk_platform_ops ops = { my_ram, my_cores, my_heat, NULL };
spk_platform_set_ops(&ops);
```

## 4. 现状与限制

- CPU 推理（AVX2）。没有 DirectML / CUDA 后端，`use_metal` 在 Windows 上无效果。
- 0.8B Q4_K_M 在 8 核桌面 CPU 上大致 12–25 tok/s，这是内存带宽受限的任务，
  不看主频。
- 模型走 mmap，首次加载有几百毫秒的页错误开销，之后常驻。
