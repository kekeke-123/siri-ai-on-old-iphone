import WidgetKit
import SwiftUI

@main
struct SiriAIWidgets: WidgetBundle {
    var body: some Widget {
        SiriAILiveActivityWidget()
    }
}

struct SiriAILiveActivityWidget: Widget {
    var body: some WidgetConfiguration {
        ActivityConfiguration(for: AssistantActivityAttributes.self) { context in
            HStack(spacing: 12) {
                Image(systemName: "sparkles")
                    .font(.system(size: 24, weight: .semibold))
                    .foregroundColor(.cyan)
                VStack(alignment: .leading, spacing: 2) {
                    Text("Siri AI").font(.system(size: 14, weight: .semibold))
                    Text(context.state.phase).font(.system(size: 12)).foregroundColor(.secondary)
                }
                Spacer()
            }
            .padding()
            .activityBackgroundTint(Color.black.opacity(0.9))
        } dynamicIsland: { context in
            DynamicIsland {
                DynamicIslandExpandedRegion(.leading) {
                    Label("Siri AI", systemImage: "sparkles")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundColor(.cyan)
                }
                DynamicIslandExpandedRegion(.trailing) {
                    Text(context.state.phase)
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
                DynamicIslandExpandedRegion(.bottom) {
                    Text(context.state.detail.isEmpty ? "本机推理中" : context.state.detail)
                        .font(.system(size: 12))
                        .lineLimit(2)
                }
            } compactLeading: {
                Image(systemName: "sparkles").foregroundColor(.cyan)
            } compactTrailing: {
                Text(context.state.phase).font(.system(size: 10)).foregroundColor(.cyan)
            } minimal: {
                Image(systemName: "waveform").foregroundColor(.cyan)
            }
            .keylineTint(.cyan)
        }
    }
}
