# 性能

> 下表的数字是**量级预估**，用于选型，不是实测。拿到真机后请用 App 设置页里的 metrics 自行校准。

## 参考吞吐（Q4_K_M、4 线程、Metal 开、上下文 2048）

| 芯片 | 代表机型 | 0.5B | 1.5B | 3B |
| --- | --- | --- | --- | --- |
| A12 | iPhone XS / XR | 18–25 tok/s | 5–8 | 2–3 |
| A13 | iPhone 11 / SE2 | 22–30 | 7–10 | 3–4 |
| A14 | iPhone 12 系列 | 26–35 | 8–12 | 3–5 |
| A15 | iPhone 13 系列 | 32–42 | 10–14 | 4–6 |
| A16 | iPhone 14 / 15 | 40–50 | 12–18 | 5–8 |

首字延迟（TTFT）：

| 场景 | 1.5B @ 2048 ctx |
| --- | --- |
| 冷启动，无前缀快照 | 1.8–4.0 s |
| `warmup()` 之后 | 120–350 ms |
| 带图提问（256M 视觉） | 300–700 ms |

## 内存预算

`max_model_gb` 默认 = 物理内存 × 0.45。以 4GB 机型为例：上限约 1.8GB，1.5B-Q4（~1.0GB）+ KV（Q8，2048 ctx，~250MB）+ 视觉（~200MB）刚好在红线内；再挂翻译模型就必须先卸视觉。

## 调参速查

| 症状 | 调整 |
| --- | --- |
| 首字太慢 | 开 `warmup()`；缩短系统提示；降 `n_ctx` |
| 生成太慢 | `n_batch=256`、`n_ubatch=64`；确认 Metal 生效；别在低电量模式 |
| 内存告警 | 降 `n_ctx` → 卸翻译 → 卸载视觉 → 换 0.5B |
| 发热降频 | 内核按 thermal 自动降 batch；UI 侧限制并发请求 |
| 长文卡顿 | 关掉历史轮数（保留最近 6 轮），或每 4 轮 `checkpoint()` |

## 已内置的优化

1. **前缀 KV 快照**：系统提示只算一次。
2. **Q8 KV cache**：KV 体积减半，延迟几乎不变。
3. **Metal + metallib 内嵌**：CI 产出单文件 IPA，无需运行时编译 shader。
4. **QoS 固定**：推理线程不被系统降到能效核（`patches/0003`）。
5. **懒加载角色模型**：启动只读主模型，省 0.3–1.0 GB 常驻。
6. **`MADV_DONTNEED` 卸载**：模型换绑时立刻归还页缓存，避免下一次加载撞上内存压力。

## 测量方法

```swift
let m = SPKAssistant.instance().metrics()
// ttft_ms / prefill_tok_s / decode_tok_s / peak_rss_mb / thermal_state
```

建议每档机型跑：冷启动一次问答、连续 10 轮、带图一次、翻译一次，记录 ttft 与 rss 峰值。

## 1.4.1 这一轮改了什么

| 改动 | 位置 | 为什么快 |
| --- | --- | --- |
| 采样链按会话缓存 | `spk_session_sampler()` | 原来的 penalties 采样器每次请求都要按词表大小分配数组并遍历整个 vocab（Qwen 系 15 万词表 → 每次约 2 MB 分配 + 15 万次写入）。现在同一组参数只建一次，`spk_sampler_reset()` 复位后复用 |
| 复用一个 1-token batch | decode 循环 | 原来每生成一个 token 就 `spk_batch_init`/`free` 一次，30 tok/s 时约 180 次 malloc/free，抖动明显 |
| token 拼接后再发 | decode 循环 | 满 96 字节或 20 ms 才跨层回调一次，C→ObjC→Swift 的桥接开销从每 token 一次降到每秒几十次 |
| Swift 侧二次合并 | `AssistantModel` | 满 48 字或 33 ms 才写 `@Published`，避免长文本每加一个字就重新排版；顺带修掉了回调线程直接改 UI 状态的问题 |
| 大内存机器加大 prefill slab | `spk_engine_create` | ≥6GB 机型 `n_batch=512 / n_ubatch=256`，长系统提示的 Metal graph 启动次数减半，TTFT 受益 |
| 灵动岛更新合并 | `LiveActivityController` | 同一状态去重 + 最短 1 s 间隔 + 尾随补发，不再每个 token 刷新一次（Dynamic Island 有每小时更新预算，超了会直接被限流） |

### 内容策略

内核不做任何提示词过滤、关键词拦截或改写：`spk_session_append()` 只做 tokenize 和模板套用，`spk_guard_*` 只管文件完整性、路径越界、上下文溢出和内存上限。模型自身的训练对齐保持不变，但没有任何额外的内容层。

### 怎么量

```swift
let m = SPKAssistant.instance().metrics()
// decode_tok_s 会比 1.4.0 略高，ttft_ms 在 ≥6GB 机型上应下降
```
