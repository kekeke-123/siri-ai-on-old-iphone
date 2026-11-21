import Foundation
#if canImport(ActivityKit)
import ActivityKit
#endif

#if canImport(ActivityKit)
@available(iOS 16.1, *)
struct AssistantActivityAttributes: ActivityAttributes {
    public struct ContentState: Codable, Hashable {
        var phase: String
        var detail: String
    }
    var title: String
}
#endif

enum LiveActivityBridge {
    static func update(phase: String, detail: String) {
        #if canImport(ActivityKit)
        if #available(iOS 16.1, *) {
            LiveActivityController.shared.update(phase: phase, detail: detail)
        }
        #endif
    }

    static func end() {
        #if canImport(ActivityKit)
        if #available(iOS 16.1, *) {
            LiveActivityController.shared.end()
        }
        #endif
    }
}

#if canImport(ActivityKit)
@available(iOS 16.1, *)
final class LiveActivityController {
    static let shared = LiveActivityController()

    private var current: Activity<AssistantActivityAttributes>?

    private init() {}

    func update(phase: String, detail: String) {
        guard ActivityAuthorizationInfo().areActivitiesEnabled else { return }

        let state = AssistantActivityAttributes.ContentState(phase: phase, detail: detail)
        let content = ActivityContent(state: state, staleDate: nil)

        if let activity = current {
            Task { await activity.update(content) }
            return
        }
        current = try? Activity.request(attributes: .init(title: "Siri AI"), content: content)
    }

    func end() {
        guard let activity = current else { return }
        current = nil
        let state = AssistantActivityAttributes.ContentState(phase: "结束", detail: "")
        Task { await activity.end(ActivityContent(state: state, staleDate: nil), dismissalPolicy: .immediate) }
    }
}
#endif
