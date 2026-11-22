import SwiftUI
import UniformTypeIdentifiers

/// System document picker wrapped directly, in copy mode.
///
/// Why not SwiftUI's `.fileImporter`: with a 500 MB payload it can close the
/// picker and never call the completion, which looks exactly like "tapping the
/// file does nothing". `asCopy: true` also makes iOS materialise the file into
/// our own tmp directory first (iCloud items are downloaded fully, no
/// security-scoped URL juggling), and the delegate always reports back.
struct ModelDocumentPicker: UIViewControllerRepresentable {
    let types: [UTType]
    let onPick: (URL) -> Void
    let onCancel: () -> Void

    func makeCoordinator() -> Coordinator { Coordinator(self) }

    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: types, asCopy: true)
        picker.delegate = context.coordinator
        picker.allowsMultipleSelection = false
        picker.shouldShowFileExtensions = true
        return picker
    }

    func updateUIViewController(_ uiViewController: UIDocumentPickerViewController, context: Context) {}

    final class Coordinator: NSObject, UIDocumentPickerDelegate {
        private let parent: ModelDocumentPicker

        init(_ parent: ModelDocumentPicker) {
            self.parent = parent
        }

        func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
            guard let url = urls.first else {
                parent.onCancel()
                return
            }
            parent.onPick(url)
        }

        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
            parent.onCancel()
        }
    }
}
