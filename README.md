# Siri AI on old iPhone

> 把新一代 Siri AI 的**界面、语音交互和系统入口**，复刻到 iPhone 15 Pro 以下的老机型上。
> 底层不是调云端 API，而是在设备上跑一个深度魔改过的本地推理运行时——**自研 LLAI 内核**。

![status](https://img.shields.io/badge/build-GitHub%20Actions-blue) ![license](https://img.shields.io/badge/license-MIT-green) ![platform](https://img.shields.io/badge/iOS-15.0%2B-lightgrey)

---

## 先说清楚能做到什么、做不到什么

| 你想要的 | 现实 | 本项目给的 |
| --- | --- | --- |
| 把国外满血版 Siri AI 的模型搬过来 | **做不到**。Apple Intelligence 的端侧模型绑定 A17 Pro 起的硬件，云端走 Private Cloud Compute，需要 Secure Enclave 认证和系统级 entitlement。没有任何可用形态的"泄露源码" | 用本地 SLM + 视觉 SLM + 翻译 SLM 组合出等价能力 |
| 国行 iPhone 打开 Apple Intelligence | 系统层面锁区 + 机型限制，第三方 App 无法改写 | 完全不依赖 Apple Intelligence，模型、语音、推理全在 App 内 |
| 界面一模一样 | 可以做到"神似"：球体、全屏对话、语音波形、系统入口 | SwiftUI 复刻的 Siri 风格界面 + 灵动岛 Live Activity |
| 长按电源键换出 Siri 后直接进 Siri AI | 电源键是系统独占，App 无法劫持 | 用 App Intents：长按电源键 → 原版 Siri → 说「打开 Siri AI」→ 直接进界面；说「问 Siri AI …」还能把问题带进来 |

**一句话**：Apple 那套搬不过来，但"老机型上有一个长得一样、听得懂、能看图、能翻译、能进灵动岛和快捷指令的本地 Siri AI"——这个项目做到了。

本项目**不包含任何 Apple 源码、模型权重或界面资源**，是干净室实现。「Siri」是 Apple Inc. 的商标，本项目与 Apple 无关联。

---

## 能力

- 全屏 Siri 风格对话界面 + 动态球体（Canvas，30fps，老机型也不掉帧）
- **纯离线语音**：Apple Speech 端上识别（`requiresOnDeviceRecognition`），音频不出设备；AVSpeechSynthesizer 朗读回答
- **多模态视觉 SLM**：图片 + 提问走多模态投影（mmproj），一次问答即可看图
- **翻译 SLM**：独立翻译模型（T5 系）或主模型指令翻译；编码器-解码器走 encoder 路径
- **音频专项**：宿主 ASR 回调接口，可替换成任意本地音频模型，不改内核
- **灵动岛**：Live Activity 显示"聆听 / 思考 / 回复中"，刘海机型回落锁屏实时活动
- **App Intents / 快捷指令**：`问 Siri AI`、`打开 Siri AI`、`用 Siri AI 翻译` 三个动作，直接在快捷指令里调用
- **URL Scheme**：`siri-ai://ask?q=你好`
- 模型文件、对话、KV 缓存全部留在沙盒内，可一键清空

---

## 架构

```
┌──────────────────────────────────────────────────────────┐
│ SwiftUI App  (AssistantView / OrbView / VoiceEngine)     │
│    ├── App Intents · Shortcuts · URL Scheme              │
│    └── Live Activity (Dynamic Island / Lock Screen)      │
├──────────────────────────────────────────────────────────┤
│ SPKBridge (ObjC++)  — 唯一的 Objective-C 边界            │
├──────────────────────────────────────────────────────────┤
│ LLAI Kernel (C ABI: spk_*)                          │
│   spk_engine   : 模型/上下文/角色槽位/热策略/内存压力     │
│   spk_session  : 模板、前缀 KV 复用、解码循环、快照       │
│   spk_vision   : 图像编码 + 多模态评估                    │
│   spk_translate: 编码器-解码器 / 指令翻译                 │
│   spk_guard    : 路径、GGUF 完整性、特殊 token 转义       │
│   spk_sampler  : 统一采样链                               │
├──────────────────────────────────────────────────────────┤
│ spk_core.h（唯一适配层，MIT 归属，见 third_party/NOTICE） │
├──────────────────────────────────────────────────────────┤
│ 上游推理运行时（构建时 fetch + patches/ 打补丁，不 vendored）│
└──────────────────────────────────────────────────────────┘
```

内核自身源码**不出现上游命名**：所有对外/对内标识都是 `spk_*`，唯一保留上游符号的文件是 `kernel/src/spk_core.h`（为了 MIT 归属与可维护性）。

### 深度魔改点（`patches/`）

| 补丁 | 内容 |
| --- | --- |
| `0001-spk-gguf-integrity.patch` | GGUF 张量/KV 数量硬上限（防解析 DoS）；张量数据段必须落在文件剩余字节内（防截断文件越界读） |
| `0002-spk-mmap-guard.patch` | 只映射普通文件、拒绝符号链接、加载前后 stat 尺寸一致性（TOCTOU）；卸载前 `MADV_DONTNEED` 归还页缓存，避免被 jetsam 杀 |
| `0003-spk-thread-qos.patch` | 推理线程固定 `QOS_CLASS_USER_INTERACTIVE`，避免后台降级到能效核导致首字延迟翻倍 |

### 内核的"自研"部分（不靠补丁）

- **前缀 KV 复用**：系统提示只 prefill 一次并快照，之后每轮从快照恢复，省掉 800+ token 的重复计算
- **Q8 KV cache**：显存/内存占用砍半，让 4096 上下文塞进 4GB 机型
- **角色槽位懒加载**：视觉、翻译、ASR 首次使用时才载入，内存警告时优先卸载
- **热管理**：按 `thermalState` 缩小 batch 而不是砍线程，避免延迟尖刺
- **安全护栏**：用户文本里的 `<|im_start|>` 等控制标记转义为全角字符，配合 `parse_special=false`，阻断模板注入

---

## 安装（免签名）

1. 打开 Actions → 最近一次 `build` → 下载 `SiriAI-unsigned-ipa`
2. 用 **TrollStore** 直接安装（永久签名，无需证书）
3. 或 AltStore / Sideloadly 自签（需要 Apple ID）
4. 首次启动：设置里导入 GGUF 模型（见下表）

> 工程本身不签名，CI 只产出未签名 IPA，签名方式由你决定。

## 模型怎么选

| 角色 | 推荐 | 体积 | 适配机型 |
| --- | --- | --- | --- |
| 主 SLM | Qwen2.5-1.5B-Instruct Q4_K_M | ~1.0 GB | 4GB 内存可跑，6GB 更稳 |
| 主 SLM（更轻） | Qwen2.5-0.5B-Instruct Q4_K_M | ~0.4 GB | iPhone 8 / SE2 也能跑 |
| 视觉 SLM | SmolVLM-256M / 500M + 对应 mmproj | 0.2–0.5 GB | 与主模型二选一使用 |
| 翻译 SLM | flan-t5-small / base（GGUF, encoder-decoder） | 0.2–0.6 GB | 懒加载，用完可卸 |

详见 [`docs/MODELS.md`](docs/MODELS.md)。

## 系统入口怎么用

**长按电源键 → 原版 Siri → 说「打开 Siri AI」**
Siri 通过 `OpenSiriAIIntent` 直接拉起 App（App Shortcut 已注册「打开 Siri AI」短语）。

**「问 Siri AI 明天义乌天气怎么样」**
`AskSiriAIIntent` 接住参数，App 启动后自动把这句话送进本地模型。

**快捷指令**：搜索「Siri AI」，三个动作可直接串进自动化。
**URL**：`siri-ai://ask?q=...`。

> 说明：电源键长按的 Siri 属于系统进程，任何侧载 App 都无法替换它。以上是 Apple 允许的、不需要越狱的接入方式。

## 自己编译

GitHub Actions（推荐，无需 Mac）：

```
Actions → build → Run workflow
```

本地（macOS + Xcode 15+）：

```bash
./scripts/fetch-runtime.sh       # 拉上游 + 打 3 个补丁
./scripts/build-ios.sh           # 编 arm64 静态库（Metal 已内嵌 metallib）
./scripts/gen-xcconfig.sh        # 生成 app/SPKVendor.xcconfig
cd app && xcodegen generate
xcodebuild -project SiriAI.xcodeproj -scheme SiriAI -sdk iphoneos \
  -configuration Release CODE_SIGNING_ALLOWED=NO build
./scripts/package-ipa.sh
```

## 代码里直接调用内核

C：

```c
#include <spk/spk.h>

spk_engine * eng = NULL;
spk_engine_config cfg = {0};
cfg.model_path = "/path/to/model.gguf";
cfg.mmproj_path = "/path/to/mmproj.gguf";   // 可选，开视觉
cfg.use_metal = true;

char err[256];
spk_engine_create(&cfg, &eng, err, sizeof(err));

spk_session * ses = NULL;
spk_session_create(eng, &ses);
spk_session_warmup(ses);                    // 系统提示只 prefill 一次

spk_session_append(ses, "user", "用一句话解释什么是 KV cache", NULL);

spk_gen_params gp = {0};
gp.max_tokens = 256;
spk_session_generate(ses, &gp, on_token, NULL);
```

Swift（通过 `SPKAssistant`）：

```swift
let err = SPKAssistant.instance().loadModel(atPath: model.path, mmprojPath: mm.path)
SPKAssistant.instance().ask("看图里有什么", images: [img.path],
                            onToken: { piece, _ in print(piece ?? "") },
                            completion: { _ in })
```

完整 API：[`docs/API.md`](docs/API.md) / [`kernel/include/spk/spk.h`](kernel/include/spk/spk.h)

---

## 文档

- [架构](docs/ARCHITECTURE.md)
- [安全模型](docs/SECURITY.md)
- [性能调优](docs/PERFORMANCE.md)
- [API](docs/API.md)
- [模型清单](docs/MODELS.md)

## 已知边界

- 不能劫持电源键/系统 Siri；只能通过 App Intents 与 Shortcuts 接入
- 灵动岛需要 iOS 16.1+ 且为刘海/岛机型；老设备回落锁屏
- 无 Apple Intelligence 的模型质量；1B 级 SLM 的能力上限就是 1B
- 未签名 IPA 的安装依赖 TrollStore/自签工具
- 暂未内置本地音频模型二进制，ASR 默认走 Apple 端上识别（可替换）

## License

代码 MIT。构建时拉取的上游运行时同样为 MIT，署名见 [`third_party/NOTICE.md`](third_party/NOTICE.md)。
