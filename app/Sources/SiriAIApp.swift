import SwiftUI

@main
struct SiriAIApp: App {
    @Environment(\.scenePhase) private var scenePhase

    init() {
        NotificationCenter.default.addObserver(forName: ProcessInfo.thermalStateDidChangeNotification,
                                               object: nil, queue: .main) { _ in
            AssistantModel.shared.updateThermal(SiriAIApp.thermalCode())
        }
    }

    var body: some Scene {
        WindowGroup {
            AssistantView()
                .onOpenURL { url in handle(url) }
                .onChange(of: scenePhase) { phase in
                    if phase == .active {
                        AssistantModel.shared.updateThermal(SiriAIApp.thermalCode())
                    }
                }
        }
    }

    private func handle(_ url: URL) {
        guard url.scheme == "siri-ai" else { return }
        let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
        let query = components?.queryItems?.first(where: { $0.name == "q" })?.value
        let text = url.host == "ask" ? query : (query ?? "")
        if let text = text, !text.isEmpty {
            AssistantModel.shared.send(text)
        }
    }

    static func thermalCode() -> Int {
        switch ProcessInfo.processInfo.thermalState {
        case .nominal:  return 0
        case .fair:     return 1
        case .serious:  return 2
        case .critical: return 3
        @unknown default: return 0
        }
    }
}
