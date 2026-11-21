import SwiftUI
import UniformTypeIdentifiers

struct AssistantView: View {
    @ObservedObject private var model = AssistantModel.shared
    @StateObject private var voice = VoiceEngine()

    @State private var input = ""
    @State private var showSettings = false
    @State private var showModelPicker = false
    @State private var showImagePicker = false
    @State private var pendingImage: URL?
    @State private var showError = false

    private var gguf: UTType {
        UTType(filenameExtension: "gguf") ?? .data
    }

    var body: some View {
        ZStack {
            background
            VStack(spacing: 0) {
                header
                ScrollViewReader { proxy in
                    ScrollView {
                        VStack(alignment: .leading, spacing: 12) {
                            ForEach(model.messages) { message in
                                bubble(message).id(message.id)
                            }
                            if !model.streaming.isEmpty {
                                bubble(ChatMessage(role: .assistant, text: model.streaming))
                            }
                            Color.clear.frame(height: 1).id("bottom")
                        }
                        .padding(.horizontal, 18)
                        .padding(.top, 8)
                    }
                    .onChange(of: model.messages.count) { _ in
                        withAnimation { proxy.scrollTo("bottom", anchor: .bottom) }
                    }
                }
                inputBar
            }
        }
        .sheet(isPresented: $showSettings) {
            SettingsView(model: model, voice: voice)
        }
        .fileImporter(isPresented: $showModelPicker,
                      allowedContentTypes: [gguf],
                      allowsMultipleSelection: false) { result in
            if case .success(let urls) = result, let url = urls.first {
                model.importModel(url, mmproj: nil)
            }
        }
        .fileImporter(isPresented: $showImagePicker,
                      allowedContentTypes: [.image],
                      allowsMultipleSelection: false) { result in
            if case .success(let urls) = result, let url = urls.first {
                pendingImage = try? ModelStore.importFile(url)
            }
        }
        .onAppear {
            voice.requestPermission()
            if let pending = Handoff.shared.take() {
                model.send(pending)
            }
            if !model.ready && ModelStore.existing().isEmpty {
                showSettings = true
            }
        }
        .onReceive(NotificationCenter.default.publisher(for: UIApplication.didReceiveMemoryWarningNotification)) { _ in
            model.memoryWarning()
        }
        .onReceive(NotificationCenter.default.publisher(for: .siriAIHandoff)) { note in
            if let q = note.object as? String { model.send(q) }
        }
        .alert(isPresented: Binding(get: { model.errorText != nil }, set: { if !$0 { model.errorText = nil } })) {
            Alert(title: Text("出错了"), message: Text(model.errorText ?? ""), dismissButton: .default(Text("好")))
        }
    }

    private var background: some View {
        LinearGradient(gradient: Gradient(colors: [
            Color(red: 0.05, green: 0.06, blue: 0.12),
            Color(red: 0.02, green: 0.02, blue: 0.05)
        ]), startPoint: .top, endPoint: .bottom)
        .ignoresSafeArea()
    }

    private var header: some View {
        HStack(spacing: 10) {
            VStack(alignment: .leading, spacing: 2) {
                Text("Siri AI")
                    .font(.system(size: 20, weight: .semibold, design: .rounded))
                    .foregroundColor(.white)
                Text(model.statusLine)
                    .font(.system(size: 11))
                    .foregroundColor(.white.opacity(0.55))
            }
            Spacer()
            if model.thinking {
                Button(action: { model.cancel() }) {
                    Image(systemName: "stop.circle.fill")
                        .font(.system(size: 22))
                        .foregroundColor(.white.opacity(0.8))
                }
            }
            Button(action: { showSettings = true }) {
                Image(systemName: "slider.horizontal.3")
                    .font(.system(size: 18, weight: .semibold))
                    .foregroundColor(.white.opacity(0.8))
            }
        }
        .padding(.horizontal, 20)
        .padding(.top, 8)
        .padding(.bottom, 4)
    }

    private func bubble(_ message: ChatMessage) -> some View {
        HStack {
            if message.role == .user { Spacer(minLength: 40) }
            VStack(alignment: .leading, spacing: 8) {
                if let path = message.imagePath,
                   let image = UIImage(contentsOfFile: path) {
                    Image(uiImage: image)
                        .resizable()
                        .scaledToFill()
                        .frame(maxWidth: 200, maxHeight: 200)
                        .clipped()
                        .cornerRadius(14)
                }
                Text(message.text)
                    .font(.system(size: 15))
                    .foregroundColor(message.role == .user ? .white : .white.opacity(0.9))
                    .fixedSize(horizontal: false, vertical: true)
            }
            .padding(.horizontal, 14)
            .padding(.vertical, 10)
            .background(
                RoundedRectangle(cornerRadius: 18, style: .continuous)
                    .fill(message.role == .user
                          ? Color(red: 0.20, green: 0.28, blue: 0.55)
                          : Color.white.opacity(0.08))
            )
            if message.role == .assistant { Spacer(minLength: 40) }
        }
    }

    private var inputBar: some View {
        VStack(spacing: 10) {
            OrbView(state: model.orb, size: 120)
                .padding(.bottom, -6)

            if let image = pendingImage, let ui = UIImage(contentsOfFile: image.path) {
                HStack {
                    Image(uiImage: ui).resizable().scaledToFill()
                        .frame(width: 44, height: 44).clipped().cornerRadius(8)
                    Text("图片已附加").font(.system(size: 12)).foregroundColor(.white.opacity(0.7))
                    Spacer()
                    Button(action: { pendingImage = nil }) {
                        Image(systemName: "xmark.circle.fill").foregroundColor(.white.opacity(0.6))
                    }
                }
                .padding(.horizontal, 20)
            }

            HStack(spacing: 10) {
                Button(action: { showImagePicker = true }) {
                    Image(systemName: "photo")
                        .font(.system(size: 18))
                        .foregroundColor(.white.opacity(0.7))
                }

                TextField("问点什么…", text: $input, onCommit: send)
                    .textFieldStyle(PlainTextFieldStyle())
                    .padding(.horizontal, 14)
                    .padding(.vertical, 10)
                    .background(RoundedRectangle(cornerRadius: 20).fill(Color.white.opacity(0.1)))
                    .foregroundColor(.white)

                Button(action: toggleMic) {
                    Image(systemName: voice.listening ? "waveform.circle.fill" : "mic.circle.fill")
                        .font(.system(size: 26))
                        .foregroundColor(voice.listening ? .pink : .white.opacity(0.8))
                }

                Button(action: send) {
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.system(size: 26))
                        .foregroundColor(input.isEmpty ? .white.opacity(0.3) : .cyan)
                }
                .disabled(input.isEmpty)
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 10)
        }
        .background(Color.black.opacity(0.001))
    }

    private func send() {
        let text = input
        input = ""
        let image = pendingImage
        pendingImage = nil
        model.send(text, imagePath: image?.path)
    }

    private func toggleMic() {
        if voice.listening {
            voice.stop()
            return
        }
        voice.start(onText: { text, final in
            input = text
            if final || text.hasSuffix("。") || text.hasSuffix("?") {
                voice.stop()
                send()
            }
        })
    }
}
