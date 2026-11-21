import Foundation
import AVFoundation
import Speech

/// On-device recognition only. If the device cannot recognise offline we say so
/// instead of silently shipping the audio to a server.
final class VoiceEngine: ObservableObject {
    @Published var listening = false
    @Published var authorized = false
    @Published var notice: String?

    private let audio = AVAudioEngine()
    private var request: SFSpeechAudioBufferRecognitionRequest?
    private var task: SFSpeechRecognitionTask?
    private var recognizer: SFSpeechRecognizer?
    private var onText: ((String, Bool) -> Void)?

    func requestPermission() {
        SFSpeechRecognizer.requestAuthorization { status in
            DispatchQueue.main.async {
                self.authorized = (status == .authorized)
                if status != .authorized {
                    self.notice = "语音识别权限没有打开"
                }
            }
        }
        AVAudioSession.sharedInstance().requestRecordPermission { granted in
            DispatchQueue.main.async {
                if !granted { self.notice = "麦克风权限没有打开" }
            }
        }
    }

    func start(language: String = "zh-CN", onText: @escaping (String, Bool) -> Void) {
        stop()
        self.onText = onText

        let rec = SFSpeechRecognizer(locale: Locale(identifier: language))
        guard let recognizer = rec, recognizer.isAvailable else {
            notice = "这台设备的离线识别不可用"
            return
        }
        self.recognizer = recognizer
        if !recognizer.supportsOnDeviceRecognition {
            notice = "当前语言不支持纯离线识别，已停止"
            return
        }

        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playAndRecord, mode: .measurement,
                                    options: [.duckOthers, .defaultToSpeaker])
            try session.setActive(true, options: .notifyOthersOnDeactivation)

            let req = SFSpeechAudioBufferRecognitionRequest()
            req.shouldReportPartialResults = true
            req.requiresOnDeviceRecognition = true
            req.taskHint = .dictation
            request = req

            let input = audio.inputNode
            let format = input.outputFormat(forBus: 0)
            input.installTap(onBus: 0, bufferSize: 1024, format: format) { buffer, _ in
                req.append(buffer)
            }

            audio.prepare()
            try audio.start()

            listening = true
            notice = nil

            task = recognizer.recognitionTask(with: req, resultHandler: { [weak self] result, error in
                guard let self = self else { return }
                if let result = result {
                    let text = result.bestTranscription.formattedString
                    let final = result.isFinal
                    DispatchQueue.main.async {
                        self.onText?(text, final)
                        if final { self.stop() }
                    }
                }
                if error != nil {
                    DispatchQueue.main.async { self.stop() }
                }
            })
        } catch {
            notice = error.localizedDescription
            stop()
        }
    }

    func stop() {
        if audio.isRunning {
            audio.stop()
            audio.inputNode.removeTap(onBus: 0)
        }
        request?.endAudio()
        task?.cancel()
        task = nil
        request = nil
        listening = false
    }
}
