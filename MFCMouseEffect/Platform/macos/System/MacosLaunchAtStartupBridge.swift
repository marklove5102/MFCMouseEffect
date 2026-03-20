import Foundation
import Darwin

private let mfxLaunchAgentLabel = "com.mfcmouseeffect.autostart"
private let mfxLaunchAgentFileName = "\(mfxLaunchAgentLabel).plist"

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

private func mfxResolveExecutablePath() -> String {
    if let bundleExe = Bundle.main.executablePath, !bundleExe.isEmpty {
        return URL(fileURLWithPath: bundleExe).resolvingSymlinksInPath().path
    }
    guard let arg0 = CommandLine.arguments.first, !arg0.isEmpty else {
        return ""
    }
    if arg0.hasPrefix("/") {
        return URL(fileURLWithPath: arg0).resolvingSymlinksInPath().path
    }
    let cwd = URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true)
    return cwd.appendingPathComponent(arg0).resolvingSymlinksInPath().path
}

private func mfxResolveLaunchAgentFileUrl() -> URL {
    let home = FileManager.default.homeDirectoryForCurrentUser
    return home
        .appendingPathComponent("Library", isDirectory: true)
        .appendingPathComponent("LaunchAgents", isDirectory: true)
        .appendingPathComponent(mfxLaunchAgentFileName, isDirectory: false)
}

private func mfxBuildLaunchAgentPlist(executablePath: String) -> [String: Any] {
    [
        "Label": mfxLaunchAgentLabel,
        "ProgramArguments": [executablePath, "--mode=tray"],
        "RunAtLoad": true,
        "KeepAlive": false,
    ]
}

private func mfxSyncLaunchAgentManifest(_ plistUrl: URL, executablePath: String) throws {
    let plist = mfxBuildLaunchAgentPlist(executablePath: executablePath)
    let data = try PropertyListSerialization.data(fromPropertyList: plist, format: .xml, options: 0)
    try data.write(to: plistUrl, options: .atomic)
}

private struct MfxLaunchCtlResult {
    let exitCode: Int32
    let stdErr: String
}

private func mfxRunLaunchCtl(_ arguments: [String]) throws -> MfxLaunchCtlResult {
    let process = Process()
    process.executableURL = URL(fileURLWithPath: "/bin/launchctl")
    process.arguments = arguments

    let errorPipe = Pipe()
    process.standardError = errorPipe

    try process.run()
    process.waitUntilExit()

    let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
    let stdErr = String(data: errorData, encoding: .utf8) ?? ""
    return MfxLaunchCtlResult(exitCode: process.terminationStatus, stdErr: stdErr)
}

private func mfxApplyLaunchAgentRuntime(enabled: Bool, plistPath: String) throws {
    let uid = getuid()
    let domain = "gui/\(uid)"
    let service = "\(domain)/\(mfxLaunchAgentLabel)"

    // Try unload first to avoid stale registration/state before re-bootstrap.
    _ = try? mfxRunLaunchCtl(["bootout", service])

    if !enabled {
        return
    }

    let bootstrap = try mfxRunLaunchCtl(["bootstrap", domain, plistPath])
    if bootstrap.exitCode == 0 {
        return
    }
    throw NSError(
        domain: "mfx.launchctl",
        code: Int(bootstrap.exitCode),
        userInfo: [NSLocalizedDescriptionKey: bootstrap.stdErr.trimmingCharacters(in: .whitespacesAndNewlines)]
    )
}

private func mfxSetLaunchAtStartup(enabled: Bool,
                                   applyRuntime: Bool,
                                   outErrorUtf8: UnsafeMutablePointer<CChar>?,
                                   outErrorCapacity: Int32) -> Int32 {
    mfxCopyCString("", outErrorUtf8, outErrorCapacity)

    let fileManager = FileManager.default
    let plistUrl = mfxResolveLaunchAgentFileUrl()
    let launchAgentsDir = plistUrl.deletingLastPathComponent()

    do {
        try fileManager.createDirectory(at: launchAgentsDir, withIntermediateDirectories: true, attributes: nil)
        if enabled {
            let executablePath = mfxResolveExecutablePath()
            guard !executablePath.isEmpty else {
                mfxCopyCString("executable_path_missing", outErrorUtf8, outErrorCapacity)
                return -1
            }
            if !fileManager.fileExists(atPath: executablePath) {
                mfxCopyCString("executable_path_not_found", outErrorUtf8, outErrorCapacity)
                return -1
            }
            try mfxSyncLaunchAgentManifest(plistUrl, executablePath: executablePath)
            if applyRuntime {
                try mfxApplyLaunchAgentRuntime(enabled: true, plistPath: plistUrl.path)
            }
        } else {
            if applyRuntime {
                try mfxApplyLaunchAgentRuntime(enabled: false, plistPath: plistUrl.path)
            }
            if fileManager.fileExists(atPath: plistUrl.path) {
                try fileManager.removeItem(at: plistUrl)
            }
        }
        return 1
    } catch {
        mfxCopyCString(error.localizedDescription, outErrorUtf8, outErrorCapacity)
        return -1
    }
}

@_cdecl("mfx_macos_set_launch_at_startup_v1")
public func mfx_macos_set_launch_at_startup_v1(
    _ enabled: Int32,
    _ outErrorUtf8: UnsafeMutablePointer<CChar>?,
    _ outErrorCapacity: Int32
) -> Int32 {
    mfxSetLaunchAtStartup(
        enabled: enabled != 0,
        applyRuntime: true,
        outErrorUtf8: outErrorUtf8,
        outErrorCapacity: outErrorCapacity)
}

@_cdecl("mfx_macos_sync_launch_at_startup_manifest_v1")
public func mfx_macos_sync_launch_at_startup_manifest_v1(
    _ enabled: Int32,
    _ outErrorUtf8: UnsafeMutablePointer<CChar>?,
    _ outErrorCapacity: Int32
) -> Int32 {
    mfxSetLaunchAtStartup(
        enabled: enabled != 0,
        applyRuntime: false,
        outErrorUtf8: outErrorUtf8,
        outErrorCapacity: outErrorCapacity)
}
