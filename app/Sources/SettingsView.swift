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

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("主模型（SLM）")) {
                    Text(model.modelLabel).font(.system(size: 13)).foregroundColor(.secondary)
                    Button("导入 GGUF 模型") { showModel = true }
                    Button("导入视觉投影 mmproj") { showMmproj = true }
                    Button("导入翻译专用模型") { showTranslate = true }
                    Text("把 GGUF 放进「文件」App 的 Siri AI 目录，或直接从 iCloud 选。1B 左右的 Q4 模型最适合 4GB 内存的机型。")
                        .font(.system(size: 11)).foregroundColor(.secondary)
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
                    Button("重新加载模型") {
                        if let first = ModelStore.existing().first {
                            model.importModel(first, mmproj: nil)
                        }
                    }
                }

                Section(footer: Text("Siri AI on old iPhone · 内核 \(kernelVersion()) · 全部推理在本机完成，模型文件与对话不会上传。")) {
                    EmptyView()
                }
            }
            .navigationBarTitle("设置", displayMode: .inline)
            .navigationBarItems(trailing: Button("完成") { presentation.wrappedValue.dismiss() })
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
        UTType(filenameExtension: "gguf") ?? .data
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
        "1.4.0"
    }
}
