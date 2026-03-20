@preconcurrency import AppKit
@preconcurrency import Foundation

public typealias MfxTrayActionCallback = @convention(c) (UnsafeMutableRawPointer?) -> Void
public typealias MfxTrayThemeSelectCallback = @convention(c) (UnsafeMutableRawPointer?, UnsafePointer<CChar>?) -> Void
public typealias MfxTrayEffectSelectCallback = @convention(c) (
    UnsafeMutableRawPointer?,
    UnsafePointer<CChar>?,
    UnsafePointer<CChar>?
) -> Void

private final class MfxTrayActionBridge: NSObject {
    private let callbackContextBits: UInt
    private let onOpenSettingsCallback: MfxTrayActionCallback?
    private let onExitCallback: MfxTrayActionCallback?
    private let onStarProjectCallback: MfxTrayActionCallback?

    init(
        callbackContextBits: UInt,
        onOpenSettingsCallback: MfxTrayActionCallback?,
        onExitCallback: MfxTrayActionCallback?,
        onStarProjectCallback: MfxTrayActionCallback?
    ) {
        self.callbackContextBits = callbackContextBits
        self.onOpenSettingsCallback = onOpenSettingsCallback
        self.onExitCallback = onExitCallback
        self.onStarProjectCallback = onStarProjectCallback
    }

    private var callbackContext: UnsafeMutableRawPointer? {
        return UnsafeMutableRawPointer(bitPattern: callbackContextBits)
    }

    @objc
    func onOpenSettings(_ sender: Any?) {
        _ = sender
        onOpenSettingsCallback?(callbackContext)
    }

    @objc
    func onExit(_ sender: Any?) {
        _ = sender
        onExitCallback?(callbackContext)
    }

    @objc
    func onOpenStarProject(_ sender: Any?) {
        _ = sender
        onStarProjectCallback?(callbackContext)
    }
}

private final class MfxTrayMenuHandle: NSObject {
    var statusItem: NSStatusItem?
    var menu: NSMenu?
    var actionBridge: MfxTrayActionBridge?

    func cleanup() {
        if let statusItem {
            NSStatusBar.system.removeStatusItem(statusItem)
            self.statusItem = nil
        }
        self.menu = nil
        self.actionBridge = nil
    }
}

private func mfxNormalizeTrayText(_ value: UnsafePointer<CChar>?, _ fallback: String) -> String {
    guard let value else {
        return fallback
    }
    let decoded = String(cString: value)
    if decoded.isEmpty {
        return fallback
    }
    return decoded
}

private func mfxIsTraySettingsAutoTriggerEnabled() -> Bool {
    let raw = ProcessInfo.processInfo.environment["MFX_TEST_TRAY_AUTO_TRIGGER_SETTINGS_ACTION"] ?? ""
    if raw.isEmpty {
        return false
    }
    let normalized = raw.lowercased()
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on"
}

private func mfxIsTrayStarAutoTriggerEnabled() -> Bool {
    let raw = ProcessInfo.processInfo.environment["MFX_TEST_TRAY_AUTO_TRIGGER_STAR_ACTION"] ?? ""
    if raw.isEmpty {
        return false
    }
    let normalized = raw.lowercased()
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on"
}

private func mfxReadTrayMenuLayoutCaptureFilePath() -> String {
    let raw = ProcessInfo.processInfo.environment["MFX_TEST_TRAY_MENU_LAYOUT_CAPTURE_FILE"] ?? ""
    if raw.isEmpty {
        return ""
    }
    return raw.trimmingCharacters(in: .whitespacesAndNewlines)
}

private func mfxWriteTrayMenuLayoutCaptureIfNeeded(
    layoutKeys: [String],
    settingsTitle: String
) {
    let capturePath = mfxReadTrayMenuLayoutCaptureFilePath()
    if capturePath.isEmpty {
        return
    }
    let hasEllipsis = settingsTitle.hasSuffix("...")
    let lines = [
        "captured=1",
        "top_level_layout_keys=\(layoutKeys.joined(separator: "|"))",
        "settings_title=\(settingsTitle)",
        "settings_title_has_ellipsis=\(hasEllipsis ? "1" : "0")",
    ]
    let content = lines.joined(separator: "\n") + "\n"
    try? content.write(toFile: capturePath, atomically: true, encoding: .utf8)
}

private func mfxAddMenuSeparatorIfNeeded(_ menu: NSMenu) {
    guard !menu.items.isEmpty else {
        return
    }
    if menu.items.last?.isSeparatorItem == true {
        return
    }
    menu.addItem(NSMenuItem.separator())
}

@MainActor
private func mfxCreateTrayMenuOnMainThread(
    starProjectTitle: String,
    settingsTitle: String,
    exitTitle: String,
    tooltip: String,
    callbackContextBits: UInt,
    onOpenSettings: MfxTrayActionCallback?,
    onExit: MfxTrayActionCallback?,
    onStarProject: MfxTrayActionCallback?
) -> UnsafeMutableRawPointer? {
    let app = NSApplication.shared
    app.setActivationPolicy(.accessory)

    let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    let actionBridge = MfxTrayActionBridge(
        callbackContextBits: callbackContextBits,
        onOpenSettingsCallback: onOpenSettings,
        onExitCallback: onExit,
        onStarProjectCallback: onStarProject
    )
    let menu = NSMenu(title: "MFCMouseEffect")
    var topLevelLayoutKeys: [String] = []

    if !starProjectTitle.isEmpty {
        let starItem = NSMenuItem(
            title: starProjectTitle,
            action: #selector(MfxTrayActionBridge.onOpenStarProject(_:)),
            keyEquivalent: ""
        )
        starItem.target = actionBridge
        menu.addItem(starItem)
        topLevelLayoutKeys.append("star")
    }

    if !settingsTitle.isEmpty && !menu.items.isEmpty {
        mfxAddMenuSeparatorIfNeeded(menu)
    }

    let settingsItem = NSMenuItem(
        title: settingsTitle,
        action: #selector(MfxTrayActionBridge.onOpenSettings(_:)),
        keyEquivalent: ","
    )
    settingsItem.keyEquivalentModifierMask = [.command]
    settingsItem.target = actionBridge
    menu.addItem(settingsItem)
    topLevelLayoutKeys.append("settings")

    menu.addItem(NSMenuItem.separator())

    let exitItem = NSMenuItem(
        title: exitTitle,
        action: #selector(MfxTrayActionBridge.onExit(_:)),
        keyEquivalent: "q"
    )
    exitItem.keyEquivalentModifierMask = [.command]
    exitItem.target = actionBridge
    menu.addItem(exitItem)
    topLevelLayoutKeys.append("exit")

    mfxWriteTrayMenuLayoutCaptureIfNeeded(layoutKeys: topLevelLayoutKeys, settingsTitle: settingsTitle)

    statusItem.menu = menu
    if let button = statusItem.button {
        button.image = nil
        button.imagePosition = .noImage
        button.title = "MFX"
        button.toolTip = tooltip
    }

    let handle = MfxTrayMenuHandle()
    handle.statusItem = statusItem
    handle.menu = menu
    handle.actionBridge = actionBridge
    return Unmanaged.passRetained(handle).toOpaque()
}

private func mfxCreateTrayMenu(
    starProjectTitle: String,
    settingsTitle: String,
    exitTitle: String,
    tooltip: String,
    callbackContextBits: UInt,
    onOpenSettings: MfxTrayActionCallback?,
    onExit: MfxTrayActionCallback?,
    onStarProject: MfxTrayActionCallback?
) -> UnsafeMutableRawPointer? {
    if Thread.isMainThread {
        let menuHandleBits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateTrayMenuOnMainThread(
                    starProjectTitle: starProjectTitle,
                    settingsTitle: settingsTitle,
                    exitTitle: exitTitle,
                    tooltip: tooltip,
                    callbackContextBits: callbackContextBits,
                    onOpenSettings: onOpenSettings,
                    onExit: onExit,
                    onStarProject: onStarProject
                )
            )
        }
        if menuHandleBits == 0 {
            return nil
        }
        return UnsafeMutableRawPointer(bitPattern: menuHandleBits)
    }

    var menuHandleBits: UInt = 0
    DispatchQueue.main.sync {
        menuHandleBits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateTrayMenuOnMainThread(
                    starProjectTitle: starProjectTitle,
                    settingsTitle: settingsTitle,
                    exitTitle: exitTitle,
                    tooltip: tooltip,
                    callbackContextBits: callbackContextBits,
                    onOpenSettings: onOpenSettings,
                    onExit: onExit,
                    onStarProject: onStarProject
                )
            )
        }
    }
    if menuHandleBits == 0 {
        return nil
    }
    return UnsafeMutableRawPointer(bitPattern: menuHandleBits)
}

@MainActor
private func mfxReleaseTrayMenuOnMainThread(_ menuHandleBits: UInt) {
    if menuHandleBits == 0 {
        return
    }
    guard let menuHandle = UnsafeMutableRawPointer(bitPattern: menuHandleBits) else {
        return
    }
    let handle = Unmanaged<MfxTrayMenuHandle>.fromOpaque(menuHandle).takeRetainedValue()
    handle.cleanup()
}

@_cdecl("mfx_macos_tray_menu_create_v1")
public func mfx_macos_tray_menu_create_v1(
    _ settingsTitleUtf8: UnsafePointer<CChar>?,
    _ exitTitleUtf8: UnsafePointer<CChar>?,
    _ tooltipUtf8: UnsafePointer<CChar>?,
    _ callbackContext: UnsafeMutableRawPointer?,
    _ onOpenSettings: MfxTrayActionCallback?,
    _ onExit: MfxTrayActionCallback?
) -> UnsafeMutableRawPointer? {
    return mfxCreateTrayMenu(
        starProjectTitle: "",
        settingsTitle: mfxNormalizeTrayText(settingsTitleUtf8, "Settings..."),
        exitTitle: mfxNormalizeTrayText(exitTitleUtf8, "Exit"),
        tooltip: mfxNormalizeTrayText(tooltipUtf8, "MFCMouseEffect"),
        callbackContextBits: UInt(bitPattern: callbackContext),
        onOpenSettings: onOpenSettings,
        onExit: onExit,
        onStarProject: nil
    )
}

@_cdecl("mfx_macos_tray_menu_create_v2")
public func mfx_macos_tray_menu_create_v2(
    _ themeTitleUtf8: UnsafePointer<CChar>?,
    _ settingsTitleUtf8: UnsafePointer<CChar>?,
    _ exitTitleUtf8: UnsafePointer<CChar>?,
    _ tooltipUtf8: UnsafePointer<CChar>?,
    _ themeValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ themeLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ themeCount: UInt32,
    _ selectedThemeValueUtf8: UnsafePointer<CChar>?,
    _ callbackContext: UnsafeMutableRawPointer?,
    _ onOpenSettings: MfxTrayActionCallback?,
    _ onExit: MfxTrayActionCallback?,
    _ onThemeSelect: MfxTrayThemeSelectCallback?
) -> UnsafeMutableRawPointer? {
    return mfxCreateTrayMenu(
        starProjectTitle: "",
        settingsTitle: mfxNormalizeTrayText(settingsTitleUtf8, "Settings..."),
        exitTitle: mfxNormalizeTrayText(exitTitleUtf8, "Exit"),
        tooltip: mfxNormalizeTrayText(tooltipUtf8, "MFCMouseEffect"),
        callbackContextBits: UInt(bitPattern: callbackContext),
        onOpenSettings: onOpenSettings,
        onExit: onExit,
        onStarProject: nil
    )
}

@_cdecl("mfx_macos_tray_menu_create_v3")
public func mfx_macos_tray_menu_create_v3(
    _ themeTitleUtf8: UnsafePointer<CChar>?,
    _ settingsTitleUtf8: UnsafePointer<CChar>?,
    _ exitTitleUtf8: UnsafePointer<CChar>?,
    _ tooltipUtf8: UnsafePointer<CChar>?,
    _ themeValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ themeLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ themeCount: UInt32,
    _ selectedThemeValueUtf8: UnsafePointer<CChar>?,
    _ clickTitleUtf8: UnsafePointer<CChar>?,
    _ clickValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ clickLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ clickCount: UInt32,
    _ selectedClickValueUtf8: UnsafePointer<CChar>?,
    _ trailTitleUtf8: UnsafePointer<CChar>?,
    _ trailValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ trailLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ trailCount: UInt32,
    _ selectedTrailValueUtf8: UnsafePointer<CChar>?,
    _ scrollTitleUtf8: UnsafePointer<CChar>?,
    _ scrollValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ scrollLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ scrollCount: UInt32,
    _ selectedScrollValueUtf8: UnsafePointer<CChar>?,
    _ holdTitleUtf8: UnsafePointer<CChar>?,
    _ holdValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ holdLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ holdCount: UInt32,
    _ selectedHoldValueUtf8: UnsafePointer<CChar>?,
    _ hoverTitleUtf8: UnsafePointer<CChar>?,
    _ hoverValuesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ hoverLabelsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ hoverCount: UInt32,
    _ selectedHoverValueUtf8: UnsafePointer<CChar>?,
    _ callbackContext: UnsafeMutableRawPointer?,
    _ onOpenSettings: MfxTrayActionCallback?,
    _ onExit: MfxTrayActionCallback?,
    _ onThemeSelect: MfxTrayThemeSelectCallback?,
    _ onEffectSelect: MfxTrayEffectSelectCallback?
) -> UnsafeMutableRawPointer? {
    return mfxCreateTrayMenu(
        starProjectTitle: "",
        settingsTitle: mfxNormalizeTrayText(settingsTitleUtf8, "Settings..."),
        exitTitle: mfxNormalizeTrayText(exitTitleUtf8, "Exit"),
        tooltip: mfxNormalizeTrayText(tooltipUtf8, "MFCMouseEffect"),
        callbackContextBits: UInt(bitPattern: callbackContext),
        onOpenSettings: onOpenSettings,
        onExit: onExit,
        onStarProject: nil
    )
}

@_cdecl("mfx_macos_tray_menu_create_v4")
public func mfx_macos_tray_menu_create_v4(
    _ starProjectTitleUtf8: UnsafePointer<CChar>?,
    _ settingsTitleUtf8: UnsafePointer<CChar>?,
    _ exitTitleUtf8: UnsafePointer<CChar>?,
    _ tooltipUtf8: UnsafePointer<CChar>?,
    _ callbackContext: UnsafeMutableRawPointer?,
    _ onOpenSettings: MfxTrayActionCallback?,
    _ onExit: MfxTrayActionCallback?,
    _ onStarProject: MfxTrayActionCallback?
) -> UnsafeMutableRawPointer? {
    return mfxCreateTrayMenu(
        starProjectTitle: mfxNormalizeTrayText(starProjectTitleUtf8, "★ Star Project"),
        settingsTitle: mfxNormalizeTrayText(settingsTitleUtf8, "Settings..."),
        exitTitle: mfxNormalizeTrayText(exitTitleUtf8, "Exit"),
        tooltip: mfxNormalizeTrayText(tooltipUtf8, "MFCMouseEffect"),
        callbackContextBits: UInt(bitPattern: callbackContext),
        onOpenSettings: onOpenSettings,
        onExit: onExit,
        onStarProject: onStarProject
    )
}

@_cdecl("mfx_macos_tray_menu_release_v1")
public func mfx_macos_tray_menu_release_v1(_ menuHandle: UnsafeMutableRawPointer?) {
    let menuHandleBits = UInt(bitPattern: menuHandle)
    if menuHandleBits == 0 {
        return
    }
    if Thread.isMainThread {
        MainActor.assumeIsolated {
            mfxReleaseTrayMenuOnMainThread(menuHandleBits)
        }
        return
    }
    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            mfxReleaseTrayMenuOnMainThread(menuHandleBits)
        }
    }
}

@_cdecl("mfx_macos_tray_menu_schedule_auto_open_settings_v1")
public func mfx_macos_tray_menu_schedule_auto_open_settings_v1(_ menuHandle: UnsafeMutableRawPointer?) {
    if mfxIsTrayStarAutoTriggerEnabled() {
        let menuHandleBits = UInt(bitPattern: menuHandle)
        if menuHandleBits != 0 {
            DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(90)) {
                guard let ptr = UnsafeMutableRawPointer(bitPattern: menuHandleBits) else {
                    return
                }
                let handle = Unmanaged<MfxTrayMenuHandle>.fromOpaque(ptr).takeUnretainedValue()
                handle.actionBridge?.onOpenStarProject(nil)
            }
        }
    }

    if !mfxIsTraySettingsAutoTriggerEnabled() {
        return
    }

    let menuHandleBits = UInt(bitPattern: menuHandle)
    if menuHandleBits == 0 {
        return
    }

    DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(110)) {
        guard let ptr = UnsafeMutableRawPointer(bitPattern: menuHandleBits) else {
            return
        }
        let handle = Unmanaged<MfxTrayMenuHandle>.fromOpaque(ptr).takeUnretainedValue()
        handle.actionBridge?.onOpenSettings(nil)
    }
}

@_cdecl("mfx_macos_tray_terminate_app_v1")
public func mfx_macos_tray_terminate_app_v1() {
    @MainActor
    func terminateOnMainThread() {
        NSApplication.shared.terminate(nil)
    }

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            terminateOnMainThread()
        }
        return
    }
    DispatchQueue.main.async {
        MainActor.assumeIsolated {
            terminateOnMainThread()
        }
    }
}
