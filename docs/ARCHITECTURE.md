# 架构

## 分层

1. **UI 层（SwiftUI）** — `AssistantView` 负责对话与输入，`OrbView` 是一个 Canvas 动画（5 个模糊色块 + 高光核心），`VoiceEngine` 负责端上语音识别与音频会话。
2. **系统集成** — `Intents.swift` 注册 App Intents / AppShortcuts；`LiveActivity.swift` 驱动灵动岛；`SiriAIApp` 处理 `siri-ai://` URL 与热状态。
3. **桥接层（ObjC++）** — `SPKBridge.mm` 是 App 与内核之间唯一的 Objective-C 边界。它把 block 回调、串行队列、NSError 与 C ABI 解耦，Swift 永远不直接碰 C。
4. **内核层（C ABI）** — `spk_engine` / `spk_session` / 视觉、翻译、ASR 模块与守卫。
5. **运行时** — `spk_core.h` 之上是构建时拉取并打补丁的上游推理运行时。

## 线程模型

- 所有生成都跑在 `ai.siriplus.spk.inference` **串行队列**上；UIKit/SwiftUI 只在主线程更新。
- 内核回调 `spk_token_cb` 在推理线程被调用，桥接层拷贝字符串后 `dispatch_async` 到主队列。
- `spk_engine_cancel` 只置原子标志，生成循环每 token 检查一次，因此取消是 O(1) 且不会阻塞 UI。
- 引擎内部用 `std::mutex` 保护槽位加载/卸载；生成时不持锁，避免内存警告与生成长时间互斥。

## 前缀 KV 复用（首字延迟的主要来源）

聊天模板每轮都会重新渲染，但系统提示部分逐字节相同。做法：

1. `spk_session_warmup()` 只渲染 system 消息 → tokenize → prefill → 用 `spk_state_read` 把 seq 0 的 KV 快照到内存，同时记下 token 序列。
2. 每次生成时，把完整 prompt 的 token 序列与快照前缀做 `memcmp`；命中就 `spk_state_write` 恢复 KV，从 `prefix_len` 开始只 prefill 新增部分。
3. 任何模板/系统提示变化都会让快照失效（`spk_session_set_system` 会清空）。

在 1B 模型 + 800 token 系统提示上，这一步通常省掉 60–75% 的首字延迟。

## 角色槽位与内存

| 槽位 | 何时加载 | 何时释放 |
| --- | --- | --- |
| TEXT | 启动时 | 进程结束 |
| VISION | 第一次带图提问 | 内存警告 / 显存不足 |
| TRANSLATE | 第一次翻译 | 内存警告 |
| ASR | 不加载模型，走宿主回调 | — |

`spk_engine_handle_memory_warning` 会先卸 VISION/TRANSLATE；如果正在生成则跳过，等系统下一次警告。`max_model_gb` 默认取物理内存的 45%，超过直接拒绝加载——宁可报错也不让 App 被 jetsam 杀掉。

## 视觉路径

`spk_describe_image` 创建一个临时会话，把图片路径交给 `mtmd` 的 bitmap 解码，文本里插入媒体标记，`mtmd_tokenize` 把标记切成图像 chunk，`mtmd_helper_eval_chunks` 完成编码 + 预填充，然后走普通采样循环。图像 token 上限被压到 1024，防止一张 12MP 照片撑爆上下文。

## 翻译路径

- 主模型是 encoder-decoder（T5 系）：tokenize → `spk_encode` → decoder start token → 采样循环。
- 否则：严格的 system 指令 + 用户文本，走普通 chat 模板；temperature 默认 0.1、top_k=1，保证译文稳定。

## App Intents 流程

```
长按电源键 → 系统 Siri
   ├─「打开 Siri AI」 → OpenSiriAIIntent → 拉起 App
   └─「问 Siri AI <问题>」 → AskSiriAIIntent → Handoff.put(问题)
                                  ↓
                        App 启动 / 已在前台
                                  ↓
                  onAppear.take() 或 Notification → model.send()
```

`Handoff` 是带锁的单槽信箱：`put` 写、`take` 读并清空，保证同一条指令最多被消费一次。
