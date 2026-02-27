import Foundation

@_cdecl("mfx_macos_show_warning_notification")
public func mfx_macos_show_warning_notification(
    _ safeTitleUtf8: UnsafePointer<CChar>?,
    _ safeMessageUtf8: UnsafePointer<CChar>?
) -> Bool {
    guard let titlePtr = safeTitleUtf8, let messagePtr = safeMessageUtf8 else {
        return false
    }

    let safeTitle = String(cString: titlePtr)
    let safeMessage = String(cString: messagePtr)
    let script = "display notification \"\(safeMessage)\" with title \"\(safeTitle)\""

    let process = Process()
    process.executableURL = URL(fileURLWithPath: "/usr/bin/osascript")
    process.arguments = ["-e", script]

    if #available(macOS 10.15, *) {
        process.standardOutput = FileHandle.nullDevice
        process.standardError = FileHandle.nullDevice
    }

    do {
        try process.run()
        process.waitUntilExit()
        return process.terminationStatus == 0
    } catch {
        return false
    }
}
