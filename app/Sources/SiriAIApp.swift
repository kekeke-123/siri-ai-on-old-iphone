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
                .task { AssistantModel.shared.loadBuiltinIfAvailable() }
                .onOpenURL { url in handle(url) }
                .onChange(of: scenePhase) { phase in
                    if phase == .active {
                        AssistantModel.shared.updateThermal(SiriAIApp.thermalCode())
                    }
                }
        }
    }

    private func handle(_ url: URL) {
        // Files app / share sheet "open in SiriAI" delivers a file URL.
        // The payload is copied into the sandbox and loaded, same path as the
        // in-app picker, so both entry points behave identically.
        if url.isFileURL {
            AssistantModel.shared.importModel(url, mmproj: nil)
            return
        }
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
