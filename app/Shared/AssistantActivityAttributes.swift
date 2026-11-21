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

/// Thin façade so the rest of the app never has to think about availability.
/// The controller itself lives in the app target only; widget builds get the
/// `SPK_WIDGET` compilation condition and therefore skip those calls.
enum LiveActivityBridge {
    static func update(phase: String, detail: String) {
        #if canImport(ActivityKit) && !SPK_WIDGET
        if #available(iOS 16.1, *) {
            LiveActivityController.shared.update(phase: phase, detail: detail)
        }
        #endif
    }

    static func end() {
        #if canImport(ActivityKit) && !SPK_WIDGET
        if #available(iOS 16.1, *) {
            LiveActivityController.shared.end()
        }
        #endif
    }
}
