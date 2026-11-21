import Foundation
import UIKit
import Darwin

/// Which bundled weight file backs a tier.
///
/// The .ipa ships at most two of these (the 2 GB release-asset limit), so every
/// caller has to cope with a tier being absent and fall back.
enum ModelTier: Int, CaseIterable, Identifiable {
    case low = 0
    case mid = 1
    case high = 2

    var id: Int { rawValue }

    var label: String {
        switch self {
        case .low:  return "Qwen2.5-0.5B"
        case .mid:  return "Qwen3.5-0.8B"
        case .high: return "Qwen3.5-2B"
        }
    }

    var shortLabel: String {
        switch self {
        case .low:  return "低配"
        case .mid:  return "中配"
        case .high: return "高配"
        }
    }

    var bundledFile: String {
        switch self {
        case .low:  return "Resources/Models/qwen2.5-0.5b-instruct-q4_k_m.gguf"
        case .mid:  return "Resources/Models/Qwen3.5-0.8B-Q4_K_M.gguf"
        case .high: return "Resources/Models/Qwen3.5-2B-Q4_K_M.gguf"
        }
    }

    static func bundledURL(_ tier: ModelTier) -> URL? {
        let url = Bundle.main.bundleURL.appendingPathComponent(tier.bundledFile)
        return FileManager.default.fileExists(atPath: url.path) ? url : nil
    }
}

/// What the user asked the app to use.
enum ModelMode: Int, CaseIterable, Identifiable {
    case auto = 0
    case low  = 1
    case mid  = 2
    case high = 3
    case custom = 4
    case api = 5

    var id: Int { rawValue }

    var title: String {
        switch self {
        case .auto:   return "自动（按任务切换）"
        case .low:    return "低配 · Qwen2.5-0.5B"
        case .mid:    return "中配 · Qwen3.5-0.8B"
        case .high:   return "高配 · Qwen3.5-2B"
        case .custom: return "自定义导入的模型"
        case .api:    return "云端 API（次要）"
        }
    }

    var tier: ModelTier? {
        switch self {
        case .low:  return .low
        case .mid:  return .mid
        case .high: return .high
        default:    return nil
        }
    }
}

/// Machine identification: "iPhone12,1" is an iPhone 11, so anything at or below
/// iPhone12,x gets the 0.5B default. Model numbers are stable across iOS versions,
/// which is why this is preferred over a RAM guess.
enum DeviceProfile {
    static let machine: String = {
        var system = utsname()
        uname(&system)
        return withUnsafeBytes(of: &system.machine) { raw in
            String(decoding: raw.prefix { $0 != 0 }, as: UTF8.self)
        }
    }()

    /// 12 for the iPhone 11 line (iPhone12,1 … iPhone12,8, incl. SE2).
    static var iphoneMajor: Int? {
        guard machine.hasPrefix("iPhone") else { return nil }
        let rest = machine.dropFirst("iPhone".count)
        let digits = rest.prefix { $0.isNumber }
        return Int(digits)
    }

    static var isIPhone11OrOlder: Bool {
        guard let major = iphoneMajor else { return false }
        return major <= 12
    }

    /// Default for the bundled fleet: old phones get 0.5B, everything newer gets 0.8B.
    static var defaultTier: ModelTier {
        isIPhone11OrOlder ? .low : .mid
    }

    static var ramGB: Double {
        Double(ProcessInfo.processInfo.physicalMemory) / 1_073_741_824.0
    }

    static var summary: String {
        let name = machine.isEmpty ? "iOS" : machine
        return "\(name) · \(String(format: "%.1f", ramGB)) GB"
    }
}

/// Cheap, deterministic task classifier. No model call, no network: this runs
/// before every prompt and must cost microseconds.
enum ModelRouter {
    enum Task {
        case simple     // one-shot system-ish commands
        case chat
        case heavy      // code, math, long-form reasoning
    }

    private static let simpleMarkers = [
        "打开", "关闭", "播放", "暂停", "几点了", "天气", "音量", "亮度",
        "提醒我", "闹钟", "翻译一下", "刚才说什么"
    ]

    private static let heavyMarkers = [
        "```", "func ", "def ", "class ", "SELECT ", "#include", "import ",
        "报错", "堆栈", "异常", "推导", "证明", "算法", "复杂度", "重构",
        "解释这段", "review", "分析一下", "总结这", "写一篇", "方案"
    ]

    static func classify(_ text: String) -> Task {
        let t = text.trimmingCharacters(in: .whitespacesAndNewlines)
        if t.isEmpty { return .simple }
        if t.count > 320 { return .heavy }

        let lowered = t.lowercased()
        for marker in heavyMarkers where lowered.contains(marker.lowercased()) {
            return .heavy
        }
        if t.count <= 14 {
            for marker in simpleMarkers where t.contains(marker) {
                return .simple
            }
            if t.count <= 6 { return .simple }
        }
        return .chat
    }

    /// Which tier we would like, ignoring what is actually installed.
    static func preferredTier(for text: String) -> ModelTier {
        switch classify(text) {
        case .simple: return .low
        case .chat:   return .mid
        case .heavy:  return .high
        }
    }

    /// Best installed tier at or below the preference; never jumps above it.
    static func resolve(_ preferred: ModelTier) -> ModelTier? {
        let installed = ModelTier.allCases.filter { ModelTier.bundledURL($0) != nil }
        if installed.contains(preferred) { return preferred }
        let below = installed.filter { $0.rawValue < preferred.rawValue }
            .max { $0.rawValue < $1.rawValue }
        if let below = below { return below }
        return installed.min { $0.rawValue < $1.rawValue }
    }
}
