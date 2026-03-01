@preconcurrency import AppKit
@preconcurrency import Foundation

private func mfxClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxColorFromArgb(_ argb: UInt32) -> NSColor {
    let alpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

@MainActor
private func mfxCreateWasmTextOverlayOnMainThread(
    x: Double,
    y: Double,
    width: Double,
    height: Double,
    fontSize: Double,
    argb: UInt32,
    text: String
) -> UnsafeMutableRawPointer? {
    guard !text.isEmpty else {
        return nil
    }
    let app = NSApplication.shared
    app.setActivationPolicy(.accessory)

    let frame = NSRect(
        x: CGFloat(x),
        y: CGFloat(y),
        width: max(1.0, CGFloat(width)),
        height: max(1.0, CGFloat(height))
    )
    let panel = NSPanel(
        contentRect: frame,
        styleMask: .borderless,
        backing: .buffered,
        defer: false
    )
    panel.isOpaque = false
    panel.backgroundColor = .clear
    panel.hasShadow = false
    panel.ignoresMouseEvents = true
    panel.hidesOnDeactivate = false
    panel.level = .statusBar
    panel.collectionBehavior = [.canJoinAllSpaces, .transient]

    let content = NSView(frame: NSRect(x: 0, y: 0, width: frame.width, height: frame.height))
    content.wantsLayer = true
    if let layer = content.layer {
        layer.backgroundColor = NSColor(calibratedWhite: 0.0, alpha: 0.58).cgColor
        layer.cornerRadius = mfxClamp(frame.height * 0.24, min: 8.0, max: 22.0)
        layer.borderWidth = 1.0
        layer.borderColor = NSColor(calibratedWhite: 1.0, alpha: 0.22).cgColor
    }
    panel.contentView = content

    let clampedFontSize = max(6.0, CGFloat(fontSize))
    let labelHeight = clampedFontSize + 6.0
    let labelY = (frame.height - labelHeight) * 0.5
    let label = NSTextField(frame: NSRect(x: 0, y: labelY, width: frame.width, height: labelHeight))
    label.isEditable = false
    label.isBezeled = false
    label.drawsBackground = false
    label.isSelectable = false
    label.alignment = .center
    label.textColor = mfxColorFromArgb(argb)
    label.font = NSFont.monospacedSystemFont(ofSize: clampedFontSize, weight: .semibold)
    label.stringValue = text
    content.addSubview(label)

    return Unmanaged.passRetained(panel).toOpaque()
}

@MainActor
private func mfxShowWasmTextOverlayOnMainThread(_ overlayHandleBits: UInt) {
    guard overlayHandleBits != 0 else {
        return
    }
    guard let ptr = UnsafeMutableRawPointer(bitPattern: overlayHandleBits) else {
        return
    }
    let panel = Unmanaged<NSWindow>.fromOpaque(ptr).takeUnretainedValue()
    panel.orderFrontRegardless()
}

@MainActor
private func mfxReleaseWasmTextOverlayOnMainThread(_ overlayHandleBits: UInt) {
    guard overlayHandleBits != 0 else {
        return
    }
    guard let ptr = UnsafeMutableRawPointer(bitPattern: overlayHandleBits) else {
        return
    }
    let panel = Unmanaged<NSWindow>.fromOpaque(ptr).takeRetainedValue()
    panel.orderOut(nil)
}

@_cdecl("mfx_macos_wasm_text_overlay_create_v1")
public func mfx_macos_wasm_text_overlay_create_v1(
    _ x: Double,
    _ y: Double,
    _ width: Double,
    _ height: Double,
    _ fontSize: Double,
    _ argb: UInt32,
    _ textUtf8: UnsafePointer<CChar>?
) -> UnsafeMutableRawPointer? {
    let text = textUtf8.map(String.init(cString:)) ?? ""
    if text.isEmpty {
        return nil
    }

    if Thread.isMainThread {
        let bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateWasmTextOverlayOnMainThread(
                    x: x,
                    y: y,
                    width: width,
                    height: height,
                    fontSize: fontSize,
                    argb: argb,
                    text: text
                )
            )
        }
        return UnsafeMutableRawPointer(bitPattern: bits)
    }

    var bits: UInt = 0
    DispatchQueue.main.sync {
        bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateWasmTextOverlayOnMainThread(
                    x: x,
                    y: y,
                    width: width,
                    height: height,
                    fontSize: fontSize,
                    argb: argb,
                    text: text
                )
            )
        }
    }
    return UnsafeMutableRawPointer(bitPattern: bits)
}

@_cdecl("mfx_macos_wasm_text_overlay_show_v1")
public func mfx_macos_wasm_text_overlay_show_v1(_ overlayHandle: UnsafeMutableRawPointer?) {
    let overlayHandleBits = UInt(bitPattern: overlayHandle)
    if overlayHandleBits == 0 {
        return
    }
    if Thread.isMainThread {
        MainActor.assumeIsolated {
            mfxShowWasmTextOverlayOnMainThread(overlayHandleBits)
        }
        return
    }
    DispatchQueue.main.async {
        MainActor.assumeIsolated {
            mfxShowWasmTextOverlayOnMainThread(overlayHandleBits)
        }
    }
}

@_cdecl("mfx_macos_wasm_text_overlay_release_v1")
public func mfx_macos_wasm_text_overlay_release_v1(_ overlayHandle: UnsafeMutableRawPointer?) {
    let overlayHandleBits = UInt(bitPattern: overlayHandle)
    if overlayHandleBits == 0 {
        return
    }
    if Thread.isMainThread {
        MainActor.assumeIsolated {
            mfxReleaseWasmTextOverlayOnMainThread(overlayHandleBits)
        }
        return
    }
    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            mfxReleaseWasmTextOverlayOnMainThread(overlayHandleBits)
        }
    }
}
