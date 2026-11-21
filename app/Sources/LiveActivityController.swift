import Foundation
#if canImport(ActivityKit)
import ActivityKit

/// App-side Live Activity driver.
///
/// Only the 16.1 request/update shape is used here: the newer `ActivityContent`
/// overloads are restricted to app extensions, so an app target that links them
/// fails to compile. Deprecation warnings are expected and harmless.
///
/// Updates are coalesced: the Dynamic Island has a hard budget of activity
/// updates per hour, and one update per token would burn through it in a single
/// answer while also costing noticeable battery.
@available(iOS 16.1, *)
final class LiveActivityController {
    static let shared = LiveActivityController()

    private var current: Activity<AssistantActivityAttributes>?
    private var lastSent = ""
    private var lastSentAt = Date.distantPast
    private var pending: (phase: String, detail: String)?
    private var flushScheduled = false

    private let minInterval: TimeInterval = 1.0

    private init() {}

    func update(phase: String, detail: String) {
        guard ActivityAuthorizationInfo().areActivitiesEnabled else { return }

        let key = phase + "|" + detail
        if key == lastSent { return }

        let elapsed = Date().timeIntervalSince(lastSentAt)
        if elapsed >= minInterval {
            send(phase: phase, detail: detail)
            return
        }

        pending = (phase, detail)
        guard !flushScheduled else { return }
        flushScheduled = true
        DispatchQueue.main.asyncAfter(deadline: .now() + (minInterval - elapsed)) { [weak self] in
            guard let self = self else { return }
            self.flushScheduled = false
            guard let next = self.pending else { return }
            self.pending = nil
            self.send(phase: next.phase, detail: next.detail)
        }
    }

    private func send(phase: String, detail: String) {
        lastSent = phase + "|" + detail
        lastSentAt = Date()

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
        pending = nil
        lastSent = ""
        let state = AssistantActivityAttributes.ContentState(phase: "已结束", detail: "")
        Task { await activity.end(using: state, dismissalPolicy: .immediate) }
    }
}
#endif
