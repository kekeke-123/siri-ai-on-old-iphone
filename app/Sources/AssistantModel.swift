import Foundation
import Combine
import AVFoundation

struct ChatMessage: Identifiable {
    enum Role: String {
        case user, assistant
    }
    let id = UUID()
    let role: Role
    var text: String
    var imagePath: String?
}

final class AssistantModel: ObservableObject {
    static let shared = AssistantModel()

    @Published var messages: [ChatMessage] = []
    @Published var streaming: String = ""
    @Published var orb: OrbState = .idle
    @Published var ready = false
    @Published var visionReady = false
    @Published var errorText: String?
    @Published var statusLine = "还没有加载模型"
    @Published var modelLabel = "未加载"
    @Published var autoSpeak = true
    @Published var thinking = false

    private let bridge = SPKAssistant.instance()
    private let synth = AVSpeechSynthesizer()

    private init() {
        messages.append(ChatMessage(role: .assistant,
                                    text: "我是 Siri AI，跑在你这台老 iPhone 上。模型完全在本地，语音不出设备。"))
    }

    // MARK: models

    func importModel(_ url: URL, mmproj: URL?) {
        do {
            let local = try ModelStore.importFile(url)
            var mm: URL?
            if let mmproj = mmproj {
                mm = try ModelStore.importFile(mmproj)
            }
            if let err = bridge.loadModel(atPath: local.path, mmprojPath: mm?.path) {
                errorText = err
                statusLine = "加载失败"
                return
            }
            ready = true
            visionReady = bridge.visionReady()
            errorText = nil
            let info = bridge.info()
            let desc = (info["desc"] as? String) ?? local.lastPathComponent
            modelLabel = desc
            statusLine = "就绪 · \(threadCount(info)) 线程 · \(context(info)) ctx"
        } catch {
            errorText = error.localizedDescription
        }
    }

    func attach(url: URL, role: Int) {
        do {
            let local = try ModelStore.importFile(url)
            if let err = bridge.attachModel(atPath: local.path, role: role, mmprojPath: nil) {
                errorText = err
                return
            }
            errorText = nil
            statusLine = "已挂载附加模型"
        } catch {
            errorText = error.localizedDescription
        }
    }

    // MARK: talking

    func send(_ text: String, imagePath: String? = nil) {
        let prompt = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !prompt.isEmpty else { return }
        guard ready else {
            errorText = "先在设置里导入一个 GGUF 小模型"
            return
        }

        messages.append(ChatMessage(role: .user, text: prompt, imagePath: imagePath))
        streaming = ""
        thinking = true
        orb = .thinking

        let images: [String] = imagePath.map { [$0] } ?? []

        bridge.ask(prompt, images: images, onToken: { [weak self] piece, _ in
            guard let self = self else { return }
            if let piece = piece {
                self.streaming += piece
            }
        }, completion: { [weak self] error in
            guard let self = self else { return }
            self.thinking = false
            let answer = self.streaming
            self.streaming = ""
            if !answer.isEmpty {
                self.messages.append(ChatMessage(role: .assistant, text: answer))
                if self.autoSpeak { self.speak(answer) }
            } else if let error = error {
                self.errorText = error.localizedDescription
            }
            self.orb = .idle
        })
    }

    func cancel() {
        bridge.cancel()
        thinking = false
        orb = .idle
    }

    func clear() {
        bridge.resetConversation()
        messages.removeAll()
        streaming = ""
        messages.append(ChatMessage(role: .assistant, text: "清空了，重新开始。"))
    }

    func speak(_ text: String) {
        guard autoSpeak else { return }
        let utterance = AVSpeechUtterance(string: text)
        utterance.voice = AVSpeechSynthesisVoice(language: Locale.preferredLanguages.first ?? "zh-CN")
        utterance.rate = 0.5
        orb = .speaking
        synth.speak(utterance)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) { [weak self] in
            guard let self = self else { return }
            if !self.synth.isSpeaking { self.orb = .idle }
        }
    }

    func updateThermal(_ state: Int) {
        bridge.updateThermal(state: state)
    }

    func memoryWarning() {
        bridge.handleMemoryWarning()
    }

    func metrics() -> [String: Any] {
        (bridge.metrics() as? [String: Any]) ?? [:]
    }

    private func threadCount(_ info: [AnyHashable: Any]) -> Int {
        (info["threads"] as? NSNumber)?.intValue ?? 0
    }

    private func context(_ info: [AnyHashable: Any]) -> Int {
        (info["nCtx"] as? NSNumber)?.intValue ?? 0
    }
}

enum ModelStore {
    static var directory: URL {
        let base = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent("Models", isDirectory: true)
    }

    static func importFile(_ url: URL) throws -> URL {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let dst = directory.appendingPathComponent(url.lastPathComponent)

        let scoped = url.startAccessingSecurityScopedResource()
        defer { if scoped { url.stopAccessingSecurityScopedResource() } }

        if FileManager.default.fileExists(atPath: dst.path) {
            try FileManager.default.removeItem(at: dst)
        }
        try FileManager.default.copyItem(at: url, to: dst)
        return dst
    }

    static func existing() -> [URL] {
        let items = (try? FileManager.default.contentsOfDirectory(at: directory,
                                                                  includingPropertiesForKeys: nil)) ?? []
        return items.filter { $0.pathExtension.lowercased() == "gguf" }.sorted { $0.lastPathComponent < $1.lastPathComponent }
    }
}
