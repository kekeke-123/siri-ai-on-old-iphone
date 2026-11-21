import Foundation
import UIKit

/// Hands a request from App Intents / Siri / Shortcuts to the running UI.
final class Handoff {
    static let shared = Handoff()

    private var pending: String?
    private let lock = NSLock()

    func put(_ query: String) {
        lock.lock()
        pending = query
        lock.unlock()
        DispatchQueue.main.async {
            NotificationCenter.default.post(name: .siriAIHandoff, object: nil)
        }
    }

    func take() -> String? {
        lock.lock()
        defer { lock.unlock() }
        let value = pending
        pending = nil
        return value
    }
}

extension Notification.Name {
    static let siriAIHandoff = Notification.Name("ai.siriplus.handoff")
}

#if canImport(AppIntents)
import AppIntents

@available(iOS 16.0, *)
struct OpenSiriAIIntent: AppIntent {
    static var title: LocalizedStringResource = "打开 Siri AI"
    static var description = IntentDescription("直接进入 Siri AI 对话界面")
    static var openAppWhenRun: Bool = true

    func perform() async throws -> some IntentResult {
        return .result()
    }
}

@available(iOS 16.0, *)
struct AskSiriAIIntent: AppIntent {
    static var title: LocalizedStringResource = "问 Siri AI"
    static var description = IntentDescription("把一句话交给本机的 Siri AI 处理")
    static var openAppWhenRun: Bool = true

    @Parameter(title: "问题")
    var request: String

    func perform() async throws -> some IntentResult {
        Handoff.shared.put(request)
        return .result()
    }
}

@available(iOS 16.0, *)
struct TranslateWithSiriAIIntent: AppIntent {
    static var title: LocalizedStringResource = "让 Siri AI 翻译"
    static var openAppWhenRun: Bool = true

    @Parameter(title: "要翻译的话")
    var text: String

    func perform() async throws -> some IntentResult {
        Handoff.shared.put("把这句话翻译成中文：\(text)")
        return .result()
    }
}

@available(iOS 16.0, *)
struct SiriAIShortcuts: AppShortcutsProvider {
    static var appShortcuts: [AppShortcut] {
        AppShortcut(intent: AskSiriAIIntent(),
                    phrases: ["问\(.applicationName) \(\\.$request)",
                              "Ask \(.applicationName) \(\\.$request)"],
                    shortTitle: "问 Siri AI",
                    systemImageName: "sparkles")
        AppShortcut(intent: OpenSiriAIIntent(),
                    phrases: ["打开\(.applicationName)",
                              "Open \(.applicationName)"],
                    shortTitle: "打开 Siri AI",
                    systemImageName: "waveform")
        AppShortcut(intent: TranslateWithSiriAIIntent(),
                    phrases: ["用\(.applicationName)翻译\(\\.$text)"],
                    shortTitle: "Siri AI 翻译",
                    systemImageName: "character.book.closed")
    }
}
#endif
