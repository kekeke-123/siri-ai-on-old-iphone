import Foundation
#if canImport(ActivityKit)
import ActivityKit

/// App-side Live Activity driver.
///
/// Only the 16.1 request/update shape is used here: the newer `ActivityContent`
/// overloads are restricted to app extensions, so an app target that links them
/// fails to compile. Deprecation warnings are expected and harmless.
@available(iOS 16.1, *)
final class LiveActivityController {
    static let shared = LiveActivityController()

    private var current: Activity<AssistantActivityAttributes>?

    private init() {}

    func update(phase: String, detail: String) {
        guard ActivityAuthorizationInfo().areActivitiesEnabled else { return }

        let state = AssistantActivityAttributes.ContentState(phase: phase, detail: detail)

        if let activity = current {
            Task { await activity.update(using: state) }
            return
        }
        current = try? Activity.request(
            attributes: AssistantActivityAttributes(title: "Siri AI"),
            contentState: state,
            pushType: nil)
    }

    func end() {
        guard let activity = current else { return }
        current = nil
        let state = AssistantActivityAttributes.ContentState(phase: "已结束", detail: "")
        Task { await activity.end(using: state, dismissalPolicy: .immediate) }
    }
}
#endif
