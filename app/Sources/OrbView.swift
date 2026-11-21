import SwiftUI

enum OrbState {
    case idle
    case listening
    case thinking
    case speaking
}

/// The Siri-style blob. One Canvas, five blurred lobes that orbit a bright core.
struct OrbView: View {
    let state: OrbState
    var size: CGFloat = 220

    var body: some View {
        TimelineView(.animation(minimumInterval: 1.0 / 30.0, paused: false)) { timeline in
            Canvas { context, canvasSize in
                let t = timeline.date.timeIntervalSinceReferenceDate
                let center = CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2)
                let radius = min(canvasSize.width, canvasSize.height) / 2

                context.addFilter(.blur(radius: radius * 0.18))

                let speed = speedFactor
                let amplitude = amplitudeFactor

                for i in 0..<5 {
                    let phase = t * speed + Double(i) * .pi * 0.4
                    let angle = phase + Double(i) * 1.25
                    let r = radius * (0.30 + 0.06 * CGFloat(i % 3)) * amplitude
                    let x = center.x + CGFloat(cos(angle)) * radius * 0.32
                    let y = center.y + CGFloat(sin(angle)) * radius * 0.32

                    let rect = CGRect(x: x - r, y: y - r, width: r * 2, height: r * 2)
                    let hue = Double(i) / 5.0
                    let color = Color(hue: 0.62 + hue * 0.28,
                                      saturation: 0.85,
                                      brightness: 0.95)
                    context.fill(Path(ellipseIn: rect), with: .color(color.opacity(0.55)))
                }

                // bright core
                let core = radius * 0.36 * amplitude
                let coreRect = CGRect(x: center.x - core, y: center.y - core,
                                      width: core * 2, height: core * 2)
                context.addFilter(.blur(radius: radius * 0.05))
                context.fill(Path(ellipseIn: coreRect),
                             with: .radialGradient(Gradient(colors: [.white, .white.opacity(0.85), .clear]),
                                                   center: center,
                                                   startRadius: 0,
                                                   endRadius: core))
            }
            .frame(width: size, height: size)
            .shadow(color: Color(red: 0.35, green: 0.55, blue: 1.0).opacity(0.55), radius: 40)
        }
        .drawingGroup()
    }

    private var speedFactor: Double {
        switch state {
        case .idle:      return 0.35
        case .listening: return 1.1
        case .thinking:  return 2.0
        case .speaking:  return 0.8
        }
    }

    private var amplitudeFactor: CGFloat {
        switch state {
        case .idle:      return 0.92
        case .listening: return 1.12
        case .thinking:  return 0.78
        case .speaking:  return 1.0
        }
    }
}
