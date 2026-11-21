import Foundation

/// Secondary path: an OpenAI-compatible /chat/completions endpoint.
/// Local inference is the product; this only exists for the cases the phone
/// genuinely cannot do (huge context, translation quality, or a broken device).
final class RemoteBackend {
    static let shared = RemoteBackend()

    struct Config {
        var baseURL: String = ""
        var apiKey: String = ""
        var model: String = "gpt-4o-mini"
    }

    private enum Key {
        static let base = "spk.remote.base"
        static let key = "spk.remote.key"
        static let model = "spk.remote.model"
    }

    var config: Config {
        get {
            let d = UserDefaults.standard
            return Config(baseURL: d.string(forKey: Key.base) ?? "",
                          apiKey: d.string(forKey: Key.key) ?? "",
                          model: d.string(forKey: Key.model) ?? "gpt-4o-mini")
        }
        set {
            let d = UserDefaults.standard
            d.set(newValue.baseURL, forKey: Key.base)
            d.set(newValue.apiKey, forKey: Key.key)
            d.set(newValue.model, forKey: Key.model)
        }
    }

    var isConfigured: Bool {
        let c = config
        return !c.baseURL.trimmingCharacters(in: .whitespaces).isEmpty
    }

    /// Streams tokens on a background task; the caller hops to main itself.
    func stream(prompt: String,
                system: String?,
                onToken: @escaping (String) -> Void,
                completion: @escaping (String?, NSError?) -> Void) {
        let cfg = config
        var base = cfg.baseURL.trimmingCharacters(in: .whitespacesAndNewlines)
        while base.hasSuffix("/") { base.removeLast() }
        if !base.hasSuffix("/chat/completions") { base += "/chat/completions" }

        guard let url = URL(string: base) else {
            completion(nil, NSError(domain: "spk.remote", code: 1,
                                    userInfo: [NSLocalizedDescriptionKey: "API 地址不合法"]))
            return
        }

        var req = URLRequest(url: url)
        req.httpMethod = "POST"
        req.timeoutInterval = 90
        req.setValue("application/json", forHTTPHeaderField: "Content-Type")
        if !cfg.apiKey.isEmpty {
            req.setValue("Bearer \(cfg.apiKey)", forHTTPHeaderField: "Authorization")
        }

        var messages: [[String: String]] = []
        if let system = system, !system.isEmpty {
            messages.append(["role": "system", "content": system])
        }
        messages.append(["role": "user", "content": prompt])

        let body: [String: Any] = [
            "model": cfg.model,
            "messages": messages,
            "stream": true
        ]
        guard let data = try? JSONSerialization.data(withJSONObject: body) else {
            completion(nil, NSError(domain: "spk.remote", code: 2,
                                    userInfo: [NSLocalizedDescriptionKey: "请求体编码失败"]))
            return
        }
        req.httpBody = data

        Task.detached(priority: .userInitiated) {
            var answer = ""
            do {
                let (bytes, response) = try await URLSession.shared.bytes(for: req)
                guard let http = response as? HTTPURLResponse else {
                    completion(nil, NSError(domain: "spk.remote", code: 3,
                                            userInfo: [NSLocalizedDescriptionKey: "没有响应"]))
                    return
                }
                guard http.statusCode == 200 else {
                    completion(nil, NSError(domain: "spk.remote", code: http.statusCode,
                                            userInfo: [NSLocalizedDescriptionKey: "HTTP \(http.statusCode)"]))
                    return
                }
                for try await line in bytes.lines {
                    guard line.hasPrefix("data:") else { continue }
                    let payload = line.dropFirst(5).trimmingCharacters(in: .whitespaces)
                    if payload == "[DONE]" { break }
                    guard let chunkData = payload.data(using: .utf8),
                          let obj = try? JSONSerialization.jsonObject(with: chunkData) as? [String: Any],
                          let choices = obj["choices"] as? [[String: Any]],
                          let delta = choices.first?["delta"] as? [String: Any],
                          let piece = delta["content"] as? String,
                          !piece.isEmpty
                    else { continue }
                    answer += piece
                    onToken(piece)
                }
                completion(answer.isEmpty ? nil : answer, nil)
            } catch {
                completion(answer.isEmpty ? nil : answer, error as NSError)
            }
        }
    }
}
