# 模型

所有模型都放在 App 沙盒的 `Documents/Models/` 下（文件 App 可直接拷进去，`UIFileSharingEnabled` 已开）。只接受 `.gguf`。

## 主模型（SPK_MODEL_TEXT）

| 模型 | 量化 | 文件 | 内存峰值 | 适用 |
| --- | --- | --- | --- | --- |
| Qwen2.5-0.5B-Instruct | Q4_K_M | ~0.4 GB | ~0.8 GB | iPhone XS/XR 起的全部机型 |
| Qwen2.5-1.5B-Instruct | Q4_K_M | ~1.0 GB | ~1.5 GB | 4GB 机型可用，6GB 更稳 |
| Qwen2.5-3B-Instruct | Q4_K_M | ~1.9 GB | ~2.6 GB | 仅 6GB+，且不建议开视觉 |
| Llama-3.2-1B-Instruct | Q4_K_M | ~0.8 GB | ~1.3 GB | 英文更好，中文一般 |

选择原则：**老机型优先 0.5B**。1B 级在 A12 上能跑，但长回答会让用户等到怀疑人生。

## 视觉（SPK_MODEL_VISION）

视觉需要**成对**：一个语言/多模态模型 + 它的 `mmproj` 投影文件。

| 组合 | 语言模型 | mmproj | 合计 |
| --- | --- | --- | --- |
| SmolVLM-256M | `SmolVLM-256M-Instruct-Q8_0.gguf` | `mmproj-SmolVLM-256M-f16.gguf` | ~0.3 GB |
| SmolVLM-500M | `...-500M-Instruct-Q8_0.gguf` | 同名 mmproj | ~0.6 GB |
| Qwen2-VL-2B | `Qwen2-VL-2B-Instruct-Q4_K_M.gguf` | `mmproj-Qwen2-VL-2B-f16.gguf` | ~1.4 GB |

规则：

- `mmproj` 必须和主模型**同源同版本**，混搭会直接输出垃圾。
- 视觉模型独占一个槽位，加载后会挤掉翻译模型（内存不够时）。
- 图像 token 上限 1024，超过的会被缩图；别传 48MP 原图。

## 翻译（SPK_MODEL_TRANSLATE）

| 模型 | 类型 | 文件 | 说明 |
| --- | --- | --- | --- |
| flan-t5-small | encoder-decoder | ~0.3 GB | 中英短句够用，速度最快 |
| flan-t5-base | encoder-decoder | ~0.6 GB | 质量明显更好 |
| opus-mt-zh-en | encoder-decoder | ~0.3 GB | 单一语向，专用质量高 |

没有挂翻译模型时，内核会用主 SLM + 强指令翻译（`temperature=0.1`），质量取决于主模型。

## 音频（SPK_MODEL_ASR）

当前稳定路径是**宿主 ASR**：App 用 Apple Speech 的端上识别（`requiresOnDeviceRecognition = true`），把文本交给内核，音频不出设备。

`SPK_MODEL_ASR` 槽位为本地音频模型预留（whisper 系 ggml 权重），接口已就绪（`spk_asr_set_host_callback` 可随时替换为本地解码器），但仓库目前不带音频模型二进制——体积大且许可证各异，交由使用者自行放置。

## 量化怎么选

- `Q4_K_M`：默认，质量/体积平衡点。
- `Q5_K_M`：多 20% 体积换更少的胡说，6GB 机型可以上。
- `Q8_0`：视觉模型常用（它本身很小）。
- `Q3_K_M` 及以下：老机型救急用，质量下降明显，不建议做默认。

## 转换与来源

模型请从 Hugging Face / ModelScope 下载**已转换好的 GGUF**。自行转换时用上游 `convert_hf_to_gguf.py`（构建时 fetched 的运行时里自带），不要手工拼张量。

许可证各不相同（Apache-2.0 / Llama 社区协议 / MIT 等），分发构建产物前请逐个确认。
