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
    @State private var nearBottom = true

    private var gguf: UTType { UTType.data }

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                conversation
                inputBar
            }
            .background(Color(uiColor: .systemBackground))
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .principal) { titleBlock }
                ToolbarItem(placement: .navigationBarTrailing) {
                    HStack(spacing: 14) {
                        Button { startNewChat() } label: {
                            Image(systemName: "square.and.pencil")
                                .font(.system(size: 16, weight: .medium))
                        }
                        .disabled(model.thinking || model.messages.isEmpty)

                        Button { showSettings = true } label: {
                            Image(systemName: "slider.horizontal.3")
                                .font(.system(size: 16, weight: .medium))
                        }
                    }
                    .foregroundColor(.primary)
                }
            }
        }
        .sheet(isPresented: $showSettings) {
            SettingsView(model: model, voice: voice)
        }
        .fullScreenCover(isPresented: $showModelPicker) {
            ModelDocumentPicker(types: [gguf], onPick: { url in
                showModelPicker = false
                model.importModel(url, mmproj: nil)
            }, onCancel: { showModelPicker = false })
            .ignoresSafeArea()
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
            if let pending = Handoff.shared.take() { model.send(pending) }
            model.refreshLocalModels()
        }
        .onReceive(NotificationCenter.default.publisher(for: UIApplication.didReceiveMemoryWarningNotification)) { _ in
            model.memoryWarning()
        }
        .onReceive(NotificationCenter.default.publisher(for: .siriAIHandoff)) { note in
            if let q = note.object as? String { model.send(q) }
        }
        .alert(isPresented: Binding(get: { model.errorText != nil },
                                    set: { if !$0 { model.errorText = nil } })) {
            Alert(title: Text("出错了"),
                  message: Text(model.errorText ?? ""),
                  dismissButton: .default(Text("好")))
        }
    }

    // MARK: - header

    private var titleBlock: some View {
        VStack(spacing: 1) {
            Text("LLAI")
                .font(.system(size: 18, weight: .semibold, design: .rounded))
                .foregroundColor(.primary)
            HStack(spacing: 4) {
                Circle()
                    .fill(model.ready ? Color.green : Color.secondary.opacity(0.5))
                    .frame(width: 5, height: 5)
                Text(model.statusLine)
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                    .lineLimit(1)
            }
        }
    }

    // MARK: - transcript

    private var conversation: some View {
        ScrollViewReader { proxy in
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 12) {
                    if model.messages.isEmpty && model.streaming.isEmpty {
                        emptyState
                    }
                    ForEach(model.messages) { message in
                        bubble(message).id(message.id)
                    }
                    if !model.streaming.isEmpty {
                        bubble(ChatMessage(role: .assistant, text: model.streaming))
                            .id("streaming")
                    }
                    Color.clear.frame(height: 1).id("bottom")
                }
                .padding(.horizontal, 16)
                .padding(.top, 12)
                .padding(.bottom, 4)
            }
            .scrollDismissesKeyboard(.interactively)
            .onChange(of: model.messages.count) { _ in
                withAnimation(.easeOut(duration: 0.2)) { proxy.scrollTo("bottom", anchor: .bottom) }
            }
            .onChange(of: model.streaming) { _ in
                proxy.scrollTo("bottom", anchor: .bottom)
            }
        }
    }

    private var emptyState: some View {
        VStack(alignment: .leading, spacing: 16) {
            if !model.ready {
                card {
                    VStack(alignment: .leading, spacing: 10) {
                        Text("还没有模型")
                            .font(.system(size: 15, weight: .semibold))
                        Text("LLAI 只负责推理，权重得你自己放。两种方式：用下面的按钮导入一个 .gguf，或者把它丢进「文件」App 里本 App 的 Models 文件夹。")
                            .font(.system(size: 13))
                            .foregroundColor(.secondary)
                            .fixedSize(horizontal: false, vertical: true)
                        HStack(spacing: 10) {
                            Button { showModelPicker = true } label: {
                                Text("导入模型")
                                    .font(.system(size: 14, weight: .medium))
                                    .padding(.horizontal, 14).padding(.vertical, 8)
                                    .background(Color.accentColor)
                                    .foregroundColor(.white)
                                    .clipShape(RoundedRectangle(cornerRadius: 9, style: .continuous))
                            }
                            Button { model.refreshLocalModels() } label: {
                                Text("刷新列表")
                                    .font(.system(size: 14))
                                    .padding(.horizontal, 12).padding(.vertical, 8)
                                    .background(Color(uiColor: .tertiarySystemFill))
                                    .foregroundColor(.primary)
                                    .clipShape(RoundedRectangle(cornerRadius: 9, style: .continuous))
                            }
                            Spacer()
                        }
                    }
                }
            } else {
                VStack(alignment: .leading, spacing: 10) {
                    Text("能做什么")
                        .font(.system(size: 13, weight: .medium))
                        .foregroundColor(.secondary)
                    ForEach(suggestions, id: \.self) { s in
                        Button { input = s } label: {
                            HStack(spacing: 8) {
                                Text(s)
                                    .font(.system(size: 13))
                                    .foregroundColor(.primary)
                                    .multilineTextAlignment(.leading)
                                Spacer(minLength: 0)
                            }
                            .padding(.horizontal, 12).padding(.vertical, 9)
                            .background(Color(uiColor: .secondarySystemBackground))
                            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
        }
        .padding(.top, 16)
    }

    private var suggestions: [String] {
        ["用一句话解释递归", "把这段翻译成英文：今天风很大", "写一个 C++ 的快速排序", "三个数的平均数怎么算"]
    }

    private func card<C: View>(@ViewBuilder _ content: () -> C) -> some View {
        content()
            .padding(14)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(uiColor: .secondarySystemBackground))
            .clipShape(RoundedRectangle(cornerRadius: 14, style: .continuous))
    }

    private func bubble(_ message: ChatMessage) -> some View {
        HStack(alignment: .top) {
            if message.role == .user { Spacer(minLength: 44) }
            VStack(alignment: .leading, spacing: 8) {
                if let path = message.imagePath, let image = UIImage(contentsOfFile: path) {
                    Image(uiImage: image)
                        .resizable()
                        .scaledToFill()
                        .frame(maxWidth: 220, maxHeight: 220)
                        .clipped()
                        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
                }
                RichText(text: message.text,
                         tint: message.role == .user ? .white : .primary,
                         isUser: message.role == .user)
            }
            .padding(.horizontal, 13)
            .padding(.vertical, 9)
            .background(
                RoundedRectangle(cornerRadius: 16, style: .continuous)
                    .fill(message.role == .user
                          ? Color.accentColor
                          : Color(uiColor: .secondarySystemBackground))
            )
            .contextMenu {
                Button {
                    UIPasteboard.general.string = message.text
                } label: { Label("复制", systemImage: "doc.on.doc") }
            }
            if message.role == .assistant { Spacer(minLength: 44) }
        }
    }

    // MARK: - composer

    private var inputBar: some View {
        VStack(spacing: 8) {
            if let image = pendingImage, let ui = UIImage(contentsOfFile: image.path) {
                HStack(spacing: 10) {
                    Image(uiImage: ui).resizable().scaledToFill()
                        .frame(width: 46, height: 46).clipped()
                        .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
                    Text("图片已附加")
                        .font(.system(size: 12)).foregroundColor(.secondary)
                    Spacer()
                    Button { pendingImage = nil } label: {
                        Image(systemName: "xmark.circle.fill").foregroundColor(.secondary)
                    }
                }
                .padding(.horizontal, 16)
            }

            HStack(alignment: .bottom, spacing: 8) {
                Button { showImagePicker = true } label: {
                    Image(systemName: "photo")
                        .font(.system(size: 17))
                        .foregroundColor(.secondary)
                        .frame(width: 34, height: 34)
                }

                TextField("问点什么…", text: $input, axis: .vertical)
                    .lineLimit(1...6)
                    .textFieldStyle(.plain)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(
                        RoundedRectangle(cornerRadius: 18, style: .continuous)
                            .fill(Color(uiColor: .tertiarySystemFill))
                    )

                if model.thinking {
                    Button { model.cancel() } label: {
                        Image(systemName: "stop.circle.fill")
                            .font(.system(size: 24))
                            .foregroundColor(.secondary)
                    }
                } else if !input.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                    Button(action: send) {
                        Image(systemName: "arrow.up.circle.fill")
                            .font(.system(size: 24))
                            .foregroundColor(Color.accentColor)
                    }
                } else {
                    Button(action: toggleMic) {
                        Image(systemName: voice.listening ? "waveform.circle.fill" : "mic.circle.fill")
                            .font(.system(size: 24))
                            .foregroundColor(voice.listening ? .pink : .secondary)
                    }
                }
            }
            .padding(.horizontal, 12)
            .padding(.top, 8)
            .padding(.bottom, 6)
        }
        .background(
            Color(uiColor: .systemBackground)
                .overlay(alignment: .top) {
                    Rectangle().fill(Color(uiColor: .separator)).frame(height: 0.5)
                }
        )
    }

    private func send() {
        let text = input.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !text.isEmpty else { return }
        input = ""
        let image = pendingImage
        pendingImage = nil
        model.send(text, imagePath: image?.path)
    }

    private func startNewChat() {
        model.cancel()
        model.startNewConversation()
    }

    private func toggleMic() {
        if voice.listening { voice.stop(); return }
        voice.start(onText: { text, final in
            input = text
            if final { voice.stop(); send() }
        })
    }
}

// MARK: - text with fenced code blocks

private struct Segment {
    let text: String
    let isCode: Bool
}

private struct RichText: View {
    let text: String
    let tint: Color
    let isUser: Bool

    private var segments: [Segment] {
        var out: [Segment] = []
        let parts = text.components(separatedBy: "```")
        for (i, part) in parts.enumerated() {
            let trimmed = part.trimmingCharacters(in: .newlines)
            if trimmed.isEmpty { continue }
            if i % 2 == 1 {
                // drop the language hint on the first line
                var body = trimmed
                if let nl = body.firstIndex(of: "\n") {
                    let head = body[body.startIndex..<nl]
                    if !head.contains(" ") && head.count < 16 {
                        body = String(body[body.index(after: nl)...])
                    }
                }
                out.append(Segment(text: body, isCode: true))
            } else {
                out.append(Segment(text: trimmed, isCode: false))
            }
        }
        return out.isEmpty ? [Segment(text: text, isCode: false)] : out
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            ForEach(Array(segments.enumerated()), id: \.offset) { _, seg in
                if seg.isCode {
                    ScrollView(.horizontal, showsIndicators: false) {
                        Text(seg.text)
                            .font(.system(size: 12.5, design: .monospaced))
                            .foregroundColor(isUser ? .white : .primary)
                            .padding(10)
                            .frame(maxWidth: .infinity, alignment: .leading)
                    }
                    .background(isUser
                                ? Color.white.opacity(0.14)
                                : Color(uiColor: .tertiarySystemFill))
                    .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
                } else {
                    Text(seg.text)
                        .font(.system(size: 15))
                        .foregroundColor(tint)
                        .fixedSize(horizontal: false, vertical: true)
                }
            }
        }
    }
}
