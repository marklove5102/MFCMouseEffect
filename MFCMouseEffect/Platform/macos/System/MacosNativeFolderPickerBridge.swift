import AppKit
import Foundation

private func mfxCopyCString(_ value: String, _ outBuffer: UnsafeMutablePointer<CChar>?, _ capacity: Int32) {
    guard let outBuffer, capacity > 0 else {
        return
    }
    let maxCopy = Int(capacity) - 1
    if maxCopy < 0 {
        return
    }

    let utf8 = value.utf8CString
    let copyCount = min(maxCopy, max(0, utf8.count - 1))
    if copyCount > 0 {
        for i in 0..<copyCount {
            outBuffer[i] = utf8[i]
        }
    }
    outBuffer[copyCount] = 0
}

private func mfxResolveInitialDirectory(_ initialPathUtf8: String) -> URL? {
    if initialPathUtf8.isEmpty {
        return nil
    }
    let expanded = NSString(string: initialPathUtf8).expandingTildeInPath
    if expanded.isEmpty {
        return nil
    }

    var isDir = ObjCBool(false)
    if FileManager.default.fileExists(atPath: expanded, isDirectory: &isDir) {
        if isDir.boolValue {
            return URL(fileURLWithPath: expanded, isDirectory: true)
        }
        return URL(fileURLWithPath: expanded).deletingLastPathComponent()
    }
    return nil
}

private func mfxPickFolderOnMainThread(titleUtf8: String, initialPathUtf8: String) -> (code: Int32, path: String, error: String) {
    NSApplication.shared.activate(ignoringOtherApps: true)

    let panel = NSOpenPanel()
    panel.canChooseFiles = false
    panel.canChooseDirectories = true
    panel.allowsMultipleSelection = false
    panel.resolvesAliases = true
    if !titleUtf8.isEmpty {
        panel.message = titleUtf8
    }
    if let initialUrl = mfxResolveInitialDirectory(initialPathUtf8) {
        panel.directoryURL = initialUrl
    }

    let result = panel.runModal()
    if result == .OK {
        guard let selectedPath = panel.url?.path, !selectedPath.isEmpty else {
            return (-1, "", "selected folder path missing")
        }
        return (1, selectedPath, "")
    }
    if result == .cancel {
        return (0, "", "cancelled")
    }
    return (-1, "", "failed to show folder dialog")
}

@_cdecl("mfx_macos_pick_folder_v1")
public func mfx_macos_pick_folder_v1(
    _ titleUtf8: UnsafePointer<CChar>?,
    _ initialPathUtf8: UnsafePointer<CChar>?,
    _ outPathUtf8: UnsafeMutablePointer<CChar>?,
    _ outPathCapacity: Int32,
    _ outErrorUtf8: UnsafeMutablePointer<CChar>?,
    _ outErrorCapacity: Int32
) -> Int32 {
    mfxCopyCString("", outPathUtf8, outPathCapacity)
    mfxCopyCString("", outErrorUtf8, outErrorCapacity)

    let title = titleUtf8.map { String(cString: $0) } ?? ""
    let initialPath = initialPathUtf8.map { String(cString: $0) } ?? ""

    let outcome: (code: Int32, path: String, error: String)
    if Thread.isMainThread {
        outcome = mfxPickFolderOnMainThread(titleUtf8: title, initialPathUtf8: initialPath)
    } else {
        var captured: (code: Int32, path: String, error: String) = (-1, "", "failed to dispatch picker to main thread")
        DispatchQueue.main.sync {
            captured = mfxPickFolderOnMainThread(titleUtf8: title, initialPathUtf8: initialPath)
        }
        outcome = captured
    }

    mfxCopyCString(outcome.path, outPathUtf8, outPathCapacity)
    mfxCopyCString(outcome.error, outErrorUtf8, outErrorCapacity)
    return outcome.code
}
