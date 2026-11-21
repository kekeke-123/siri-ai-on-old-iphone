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
    @Published var mode: ModelMode = .auto
    @Published var activeTier: ModelTier?

    private let bridge = SPKAssistant.instance()
    private let synth = AVSpeechSynthesizer()

    private var pendingPiece = ""
    private var flushWork: DispatchWorkItem?
    private var builtinAttempted = false

    private init() {
        if let raw = UserDefaults.standard.object(forKey: "spk.model.mode") as? Int,
           let saved = ModelMode(rawValue: raw) {
            mode = saved
        }
        messages.append(ChatMessage(role: .assistant,
                                    text: "我是 Siri AI，跑在你这台老 iPhone 上。模型完全在本地，语音不出设备。"))
    }

    // MARK: models

    func importModel(_ url: URL, mmproj: URL?) {
        statusLine = "正在导入模型…"
        errorText = nil
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                let local = try ModelStore.importFile(url)
                var mm: URL?
                if let mmproj = mmproj {
                    mm = try ModelStore.importFile(mmproj)
                }
                DispatchQueue.main.async {
                    self.statusLine = "正在加载 \(local.lastPathComponent)…"
                }
                if let err = self.bridge.loadModel(atPath: local.path, mmprojPath: mm?.path) {
                    DispatchQueue.main.async {
                        self.errorText = err
                        self.statusLine = "加载失败"
                    }
                    return
                }
                let info = self.bridge.info()
                let desc = (info["desc"] as? String) ?? local.lastPathComponent
                let threads = self.threadCount(info)
                let ctx = self.context(info)
                let vision = self.bridge.visionReady
                DispatchQueue.main.async {
                    self.ready = true
                    self.activeTier = nil
                    self.mode = .custom
                    UserDefaults.standard.set(ModelMode.custom.rawValue, forKey: "spk.model.mode")
                    self.visionReady = vision
                    self.errorText = nil
                    self.modelLabel = desc
                    self.statusLine = "就绪 · \(threads) 线程 · \(ctx) ctx"
                }
            } catch {
                DispatchQueue.main.async {
                    self.errorText = error.localizedDescription
                    self.statusLine = "导入失败"
                }
            }
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

    // MARK: bundled tiers

    func loadBuiltinIfAvailable() {
        guard !ready, !builtinAttempted else { return }
        builtinAttempted = true
        if mode == .api {
            statusLine = RemoteBackend.shared.isConfigured ? "API 模式" : "还没配 API"
            return
        }
        let wanted = mode.tier ?? DeviceProfile.defaultTier
        guard let tier = ModelRouter.resolve(wanted), let url = ModelTier.bundledURL(tier) else {
            statusLine = "没有可用的内置模型"
            return
        }
        load(tier: tier, url: url)
    }

    func setMode(_ newMode: ModelMode) {
        mode = newMode
        UserDefaults.standard.set(newMode.rawValue, forKey: "spk.model.mode")
        builtinAttempted = true

        if newMode == .api {
            ready = false
            activeTier = nil
            statusLine = RemoteBackend.shared.isConfigured ? "API 模式（本地停用）" : "API 模式，先去下面填地址"
            return
        }
        if newMode == .custom { return }

        guard let tier = newMode.tier ?? ModelRouter.resolve(DeviceProfile.defaultTier),
              let url = ModelTier.bundledURL(tier) else {
            statusLine = "这个包没带该档位的模型"
            return
        }
        if ready && activeTier == tier { return }
        ready = false
        load(tier: tier, url: url)
    }

    private func load(tier: ModelTier, url: URL, then: (() -> Void)? = nil) {
        statusLine = "\(tier.shortLabel) \(tier.label) 加载中…"
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self = self else { return }
            let loadError = self.bridge.loadModel(atPath: url.path, mmprojPath: nil)
            DispatchQueue.main.async {
                if let loadError = loadError {
                    self.errorText = loadError
                    self.statusLine = "\(tier.shortLabel) 加载失败"
                    self.ready = false
                    return
                }
                self.ready = true
                self.activeTier = tier
                self.visionReady = self.bridge.visionReady
                self.modelLabel = tier.label
                let info = self.bridge.info() as [AnyHashable: Any]
                self.statusLine = "\(tier.shortLabel) \(tier.label) · \(self.threadCount(info)) 线程 · \(self.context(info)) ctx"
                then?()
            }
        }
    }

    // MARK: talking

    func send(_ text: String, imagePath: String? = nil) {
        let prompt = text.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !prompt.isEmpty else { return }

        if mode == .api || (!ready && RemoteBackend.shared.isConfigured) {
            appendUser(prompt, imagePath: imagePath)
            askRemote(prompt)
            return
        }

        guard ready else {
            errorText = "设置页导入一个 GGUF，或者填一个 API 地址"
            return
        }

        if mode == .auto {
            let preferred = ModelRouter.preferredTier(for: prompt)
            if let tier = ModelRouter.resolve(preferred),
               tier != activeTier,
               let url = ModelTier.bundledURL(tier) {
                statusLine = "自动切到 \(tier.shortLabel) \(tier.label)"
                ready = false
                load(tier: tier, url: url) { [weak self] in
                    self?.startAsk(prompt, imagePath: imagePath)
                }
                return
            }
        }

        startAsk(prompt, imagePath: imagePath)
    }

    private func startAsk(_ prompt: String, imagePath: String?) {
        appendUser(prompt, imagePath: imagePath)

        let images: [String] = imagePath.map { [$0] } ?? []

        bridge.ask(prompt, images: images, onToken: { [weak self] piece, _ in
            guard let self = self, let piece = piece, !piece.isEmpty else { return }
            DispatchQueue.main.async { self.enqueuePiece(piece) }
        }, completion: { [weak self] error in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.flushPiece()
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
            }
        })
    }

    func cancel() {
        bridge.cancel()
        thinking = false
        orb = .idle
    }

    private func enqueuePiece(_ piece: String) {
        pendingPiece += piece
        if pendingPiece.count >= 48 {
            flushPiece()
            return
        }
        guard flushWork == nil else { return }
        let work = DispatchWorkItem { [weak self] in
            self?.flushWork = nil
            self?.flushPiece()
        }
        flushWork = work
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.033, execute: work)
    }

    private func flushPiece() {
        flushWork?.cancel()
        flushWork = nil
        guard !pendingPiece.isEmpty else { return }
        streaming += pendingPiece
        pendingPiece = ""
    }

    private func appendUser(_ prompt: String, imagePath: String?) {
        messages.append(ChatMessage(role: .user, text: prompt, imagePath: imagePath))
        streaming = ""
        thinking = true
        orb = .thinking
    }

    private func askRemote(_ prompt: String) {
        RemoteBackend.shared.stream(prompt: prompt, system: nil, onToken: { [weak self] piece in
            DispatchQueue.main.async { self?.enqueuePiece(piece) }
        }, completion: { [weak self] answer, error in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.flushPiece()
                self.thinking = false
                let text: String = answer ?? self.streaming
                self.streaming = ""
                if !text.isEmpty {
                    self.messages.append(ChatMessage(role: .assistant, text: text))
                    if self.autoSpeak { self.speak(text) }
                } else if let error = error {
                    self.errorText = error.localizedDescription
                }
                self.orb = .idle
            }
        })
    }

    var remoteConfig: RemoteBackend.Config {
        get { RemoteBackend.shared.config }
        set { RemoteBackend.shared.config = newValue }
    }

    func clear() {
        bridge.resetConversation()
        flushWork?.cancel()
        flushWork = nil
        pendingPiece = ""
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
        bridge.updateThermalState(state)
    }

    func memoryWarning() {
        bridge.handleMemoryWarning()
    }

    func metrics() -> [String: Any] {
        bridge.metrics()
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
