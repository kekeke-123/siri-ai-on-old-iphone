import SwiftUI
import UniformTypeIdentifiers

struct SettingsView: View {
    @ObservedObject var model: AssistantModel
    @ObservedObject var voice: VoiceEngine

    @Environment(\.presentationMode) private var presentation

    @State private var picking: PickTarget?
    @State private var pickedMmproj: URL?

    private enum PickTarget: String, Identifiable {
        case model, mmproj, translate
        var id: String { rawValue }
    }

    @State private var apiBase = ""
    @State private var apiKey = ""
    @State private var apiModel = "gpt-4o-mini"

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("模型"), footer: Text("默认按机型选：iPhone 11 / SE2 及更老用 0.5B，更新的用 0.8B。自动模式会按任务在低/中/高之间切换。")) {
                    Picker("模式", selection: Binding(get: { model.mode }, set: { model.setMode($0) })) {
                        ForEach(ModelMode.allCases) { Text($0.title).tag($0) }
                    }
                    row("机型", DeviceProfile.summary)
                    row("当前模型", model.modelLabel)
                    row("当前档位", model.activeTier?.shortLabel ?? "—")
                    ForEach(ModelTier.allCases) { tier in
                        row("内置 \(tier.shortLabel)", ModelTier.bundledURL(tier) == nil ? "未随包" : tier.label)
                    }
                }

                Section(header: Text("推理后端"),
                        footer: Text("默认走 Metal（权重在 GPU，K/V 与注意力留在 CPU）。若某个模型在 GPU 上仍解码失败，关掉开关重新加载即可退回 CPU。")) {
                    Toggle("Metal 加速", isOn: Binding(
                        get: {
                            let d = UserDefaults.standard
                            return d.object(forKey: "spk.metal.enabled") == nil
                                ? true
                                : d.bool(forKey: "spk.metal.enabled")
                        },
                        set: { UserDefaults.standard.set($0, forKey: "spk.metal.enabled") }
                    ))
                    Button("重新加载当前模型") { model.reload() }
                }

                Section(header: Text("Models 文件夹（直接拖进去）"),
                        footer: Text("路径：「文件」App → 我的 iPhone → \(Bundle.main.displayName) → Models。把 .gguf 拖进去，回到 App 会自动识别并加载。")) {
                    if model.localModels.isEmpty {
                        Text("还没有文件").font(.system(size: 12)).foregroundColor(.secondary)
                    } else {
                        ForEach(model.localModels, id: \.self) { url in
                            Button {
                                model.loadLocal(url)
                            } label: {
                                HStack {
                                    Text(url.lastPathComponent)
                                        .font(.system(size: 12, design: .monospaced))
                                        .lineLimit(1)
                                    Spacer()
                                    Text(sizeText(url))
                                        .font(.system(size: 11))
                                        .foregroundColor(.secondary)
                                }
                            }
                        }
                    }
                    Button("刷新列表") { model.refreshLocalModels() }
                }

                Section(header: Text("导入 / 自定义")) {
                    Button("导入 GGUF 模型") { picking = .model }
                    Button("导入视觉投影 mmproj") { picking = .mmproj }
                    Button("导入翻译专用模型") { picking = .translate }
                    Text("导入后自动切到「自定义」，覆盖内置权重；文件会拷进 App 沙盒的 Models 目录。\n也可以在「文件」App 里长按 gguf → 分享 → 更多 → SiriAI，直接推进来。")
                        .font(.system(size: 11)).foregroundColor(.secondary)
                }

                Section(header: Text("云端 API（次要）"), footer: Text("本地永远优先。只有选「云端 API」或本地没模型可加载时，才会走这里。地址填 OpenAI 兼容的 /v1 前缀即可。")) {
                    TextField("https://api.example.com/v1", text: $apiBase)
                        .keyboardType(.URL)
                        .textInputAutocapitalization(.never)
                        .disableAutocorrection(true)
                    SecureField("API Key（可留空）", text: $apiKey)
                    TextField("模型名，如 gpt-4o-mini", text: $apiModel)
                        .textInputAutocapitalization(.never)
                        .disableAutocorrection(true)
                    Button("保存 API 设置") {
                        var cfg = model.remoteConfig
                        cfg.baseURL = apiBase.trimmingCharacters(in: .whitespacesAndNewlines)
                        cfg.apiKey = apiKey.trimmingCharacters(in: .whitespacesAndNewlines)
                        cfg.model = apiModel.trimmingCharacters(in: .whitespacesAndNewlines)
                        model.remoteConfig = cfg
                    }
                    row("状态", RemoteBackend.shared.isConfigured ? "已配置" : "未配置")
                }

                Section(header: Text("语音")) {
                    Toggle("回答朗读出来", isOn: $model.autoSpeak)
                    Text(voice.authorized ? "离线语音识别已授权" : "尚未获得语音识别权限")
                        .font(.system(size: 12)).foregroundColor(.secondary)
                    if let notice = voice.notice {
                        Text(notice).font(.system(size: 12)).foregroundColor(.orange)
                    }
                }

                Section(header: Text("性能")) {
                    let m = model.metrics()
                    row("首字延迟", "\(Int((m["ttft"] as? NSNumber)?.doubleValue ?? 0)) ms")
                    row("预填充", String(format: "%.1f tok/s", (m["prefill"] as? NSNumber)?.doubleValue ?? 0))
                    row("生成", String(format: "%.1f tok/s", (m["decode"] as? NSNumber)?.doubleValue ?? 0))
                    row("内存峰值", String(format: "%.0f MB", (m["rss"] as? NSNumber)?.doubleValue ?? 0))
                    row("热状态", thermalLabel(Int((m["thermal"] as? NSNumber)?.intValue ?? 0)))
                }

                Section(header: Text("会话")) {
                    Button("清空对话") { model.clear() }
                    Button("重新加载当前模型") { model.setMode(model.mode) }
                }

                Section(footer: Text("Siri AI on old iPhone · 内核 \(kernelVersion()) · 默认全程本机推理，模型文件与对话不上传。")) {
                    EmptyView()
                }
            }
            .navigationBarTitle("设置", displayMode: .inline)
            .navigationBarItems(trailing: Button("完成") { presentation.wrappedValue.dismiss() })
            .onAppear {
                let cfg = model.remoteConfig
                apiBase = cfg.baseURL
                apiKey = cfg.apiKey
                apiModel = cfg.model
            }
            .fullScreenCover(item: $picking) { target in
                ModelDocumentPicker(types: [ggufType], onPick: { url in
                    let kind = target
                    picking = nil
                    switch kind {
                    case .model:
                        model.importModel(url, mmproj: pickedMmproj)
                    case .mmproj:
                        pickedMmproj = try? ModelStore.importFile(url)
                    case .translate:
                        model.attach(url: url, role: 2)
                    }
                }, onCancel: {
                    picking = nil
                })
                .ignoresSafeArea()
            }
        }
    }

    private var ggufType: UTType {
        UTType.data // .gguf has no registered UTI; a dynamic one leaves the file untappable
    }

    private func row(_ title: String, _ value: String) -> some View {
        HStack {
            Text(title).font(.system(size: 13))
            Spacer()
            Text(value).font(.system(size: 13, design: .monospaced)).foregroundColor(.secondary)
        }
    }

    private func thermalLabel(_ v: Int) -> String {
        switch v {
        case 1: return "偏热"
        case 2: return "降频中"
        case 3: return "严重降频"
        default: return "正常"
        }
    }

    private func sizeText(_ url: URL) -> String {
        let attrs = try? FileManager.default.attributesOfItem(atPath: url.path)
        let bytes = (attrs?[.size] as? NSNumber)?.int64Value ?? 0
        let mb = Double(bytes) / 1_048_576
        return mb >= 1024 ? String(format: "%.1f GB", mb / 1024) : String(format: "%.0f MB", mb)
    }

    private func kernelVersion() -> String {
        "1.4.2"
    }
}

private extension Bundle {
    var displayName: String {
        (object(forInfoDictionaryKey: "CFBundleDisplayName") as? String)
            ?? (object(forInfoDictionaryKey: "CFBundleName") as? String)
            ?? "Siri AI"
    }
}
