import SwiftUI
import UniformTypeIdentifiers

struct SettingsView: View {
    @ObservedObject var model: AssistantModel
    @ObservedObject var voice: VoiceEngine

    @Environment(\.presentationMode) private var presentation

    @State private var showModel = false
    @State private var showMmproj = false
    @State private var showTranslate = false
    @State private var pickedMmproj: URL?

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

                Section(header: Text("导入 / 自定义")) {
                    Button("导入 GGUF 模型") { showModel = true }
                    Button("导入视觉投影 mmproj") { showMmproj = true }
                    Button("导入翻译专用模型") { showTranslate = true }
                    Text("导入后自动切到「自定义」，覆盖内置权重；文件会拷进 App 沙盒的 Models 目录。")
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
            .fileImporter(isPresented: $showModel, allowedContentTypes: [ggufType], allowsMultipleSelection: false) { r in
                if case .success(let urls) = r, let u = urls.first {
                    model.importModel(u, mmproj: pickedMmproj)
                }
            }
            .fileImporter(isPresented: $showMmproj, allowedContentTypes: [ggufType], allowsMultipleSelection: false) { r in
                if case .success(let urls) = r, let u = urls.first {
                    pickedMmproj = try? ModelStore.importFile(u)
                }
            }
            .fileImporter(isPresented: $showTranslate, allowedContentTypes: [ggufType], allowsMultipleSelection: false) { r in
                if case .success(let urls) = r, let u = urls.first {
                    model.attach(url: u, role: 2)
                }
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

    private func kernelVersion() -> String {
        "1.4.2"
    }
}
