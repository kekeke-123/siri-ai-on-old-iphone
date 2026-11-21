# API

两套入口：C ABI（`spk.h`）给 C/C++，`SPKAssistant` 给 Swift/ObjC。Swift 请只用后者。

## 语义约定

- 所有字符串 UTF-8；失败时返回 `spk_status`，详细原因在 `spk_engine_last_error`。
- 回调里的 buffer 只在调用期间有效，要留就自己拷贝。
- 一个 `spk_engine` 就是一个独立运行时，没有全局状态，可以并存多个。
- 除 `spk_engine_cancel` 与 `spk_engine_set_thermal_state` 外，同一 engine 上的调用需串行。

## 生命周期

```c
char err[256];
spk_engine_config cfg = {0};
cfg.model_path   = "/var/.../Documents/Models/main.gguf";
cfg.mmproj_path  = "/var/.../Documents/Models/mmproj.gguf";  // 可 NULL
cfg.system_prompt = "你是 Siri AI，回答简短直接。";
cfg.secure_mode  = true;

spk_engine *eng = NULL;
if (spk_engine_create(&cfg, &eng, err, sizeof(err)) != SPK_OK) { /* err 里是原因 */ }

spk_engine_add_model(eng, SPK_MODEL_TRANSLATE, "/.../t5-small.gguf", NULL);

spk_session *ses = NULL;
spk_session_create(eng, &ses);
spk_session_warmup(ses);          // 失败可忽略，只是没有快照
```

销毁顺序：session → engine。

## 生成

```c
spk_session_append(ses, "user", "解释一下 Metal 是什么", NULL);

spk_gen_params gp = {
    .max_tokens = 512, .top_k = 40, .top_p = 0.95,
    .min_p = 0.05, .temperature = 0.7,
    .repeat_penalty = 1.1, .repeat_last_n = 64, .seed = 0,
};

spk_session_generate(ses, &gp, [](const char *piece, bool fin, void *ud) {
    // piece 是增量文本，fin 为 true 表示结束
}, NULL);

// 记得把答案作为 assistant 回合写回会话，否则下一轮没有上下文
spk_session_append(ses, "assistant", answer, NULL);
```

`images` 参数是 NULL 结尾的路径数组；带图时该回合会走多模态前缀。

## 视觉

```c
if (spk_supports_vision(eng))
    spk_describe_image(eng, "/.../photo.jpg", "图里有什么", &gp, cb, NULL);
```

独立会话，不污染聊天历史。

## 翻译

```c
spk_translate(eng, "hello world", "en", "zh", &gp, cb, NULL);
```

挂载了 `SPK_MODEL_TRANSLATE` 就走专用模型，否则用主模型 + 严格指令。

## 音频

```c
char * host_asr(const char *path, const char *lang, void *ud) {
    return strdup("识别出来的文本");   // 由内核负责 free
}
spk_asr_set_host_callback(eng, host_asr, NULL);

char out[1024];
spk_asr_transcribe(eng, "/.../clip.m4a", "zh", out, sizeof(out));
```

App 里默认接的就是 Apple 端上识别，音频不出设备。

## 运行时控制

| 调用 | 用途 |
| --- | --- |
| `spk_engine_cancel` | 立刻中断生成（线程安全） |
| `spk_engine_set_thermal_state` | 0–3，热了自动降 batch |
| `spk_engine_handle_memory_warning` | 卸掉视觉/翻译，保住主模型 |
| `spk_session_checkpoint/restore` | 整段序列快照，适合"回到某一轮" |
| `spk_session_reset` | 清空回合与 KV，保留系统提示 |
| `spk_engine_info / metrics` | 参数量、上下文、ttft、tok/s、RSS |

## 状态码

`SPK_OK` / `INVALID_ARGUMENT` / `NOT_FOUND` / `UNSUPPORTED` / `MODEL_REJECTED`（守卫在加载前拒绝）/ `MODEL_LOAD` / `CONTEXT` / `OUT_OF_MEMORY` / `CONTEXT_OVERFLOW` / `CANCELLED` / `THERMAL` / `INTERNAL` / `VISION_UNAVAILABLE` / `ASR_UNAVAILABLE`。

## Swift 映射

| ObjC | Swift |
| --- | --- |
| `+instance` | `SPKAssistant.instance()` |
| `loadModelAtPath:mmprojPath:` | `loadModel(atPath:mmprojPath:)` → `String?`（nil 成功） |
| `attachModelAtPath:role:mmprojPath:` | `attachModel(atPath:role:mmprojPath:)` |
| `ask:images:onToken:completion:` | `ask(_:images:onToken:completion:)` |
| `metrics` | `metrics()` → `[String: Any]` |

role 取值：0 文本（等价于 `loadModel`）、1 视觉、2 翻译、3 ASR。

```swift
SPKAssistant.instance().ask("你好", images: nil,
    onToken: { piece, _ in text += piece ?? "" },
    completion: { err in if let err { print(err) } })
```
