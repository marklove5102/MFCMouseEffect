@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(_ x: Int32, _ y: Int32) -> Int32

private func mfxClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxColorFromArgb(_ argb: UInt32, alphaScale: CGFloat) -> NSColor {
    let baseAlpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let alpha = mfxClamp(baseAlpha * alphaScale, min: 0.0, max: 1.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxResolveOverlayTargetFps(overlayX: Int32, overlayY: Int32) -> Float {
    let intervalMs = Int(mfxOverlayTimerIntervalMs(overlayX, overlayY))
    if intervalMs <= 0 {
        return 60.0
    }
    let fps = max(1, min(240, Int((1000.0 / Double(intervalMs)).rounded())))
    return Float(fps)
}

@available(macOS 12.0, *)
private func mfxApplyPreferredFrameRate(_ animation: CAAnimation, targetFps: Float) {
    let fps = max(1.0, min(240.0, targetFps))
    animation.preferredFrameRateRange = CAFrameRateRange(
        minimum: fps,
        maximum: fps,
        preferred: fps
    )
}

private func mfxBuildPolylinePath(
    pointsXY: UnsafePointer<Float>,
    pointCount: Int,
    closed: Bool
) -> CGPath? {
    guard pointCount >= 2 else {
        return nil
    }
    let path = CGMutablePath()
    let first = CGPoint(x: CGFloat(pointsXY[0]), y: CGFloat(pointsXY[1]))
    path.move(to: first)
    var offset = 2
    for _ in 1..<pointCount {
        let point = CGPoint(x: CGFloat(pointsXY[offset]), y: CGFloat(pointsXY[offset + 1]))
        path.addLine(to: point)
        offset += 2
    }
    if closed {
        path.closeSubpath()
    }
    return path
}

@MainActor
private func mfxCreateWasmPolylineOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    pointsXY: UnsafePointer<Float>,
    pointCount: Int,
    lineWidthPx: Double,
    strokeArgb: UInt32,
    glowArgb: UInt32,
    alphaScale: Double,
    durationSec: Double,
    closed: Bool
) -> UnsafeMutableRawPointer? {
    guard let path = mfxBuildPolylinePath(pointsXY: pointsXY, pointCount: pointCount, closed: closed) else {
        return nil
    }

    let size = max(1.0, CGFloat(frameSize))
    let frame = NSRect(x: frameX, y: frameY, width: size, height: size)
    let overlayX = Int32((frameX + frameSize * 0.5).rounded())
    let overlayY = Int32((frameY + frameSize * 0.5).rounded())
    let targetFps = mfxResolveOverlayTargetFps(overlayX: overlayX, overlayY: overlayY)

    let window = NSWindow(
        contentRect: frame,
        styleMask: .borderless,
        backing: .buffered,
        defer: false
    )
    window.isOpaque = false
    window.backgroundColor = .clear
    window.hasShadow = false
    window.ignoresMouseEvents = true
    window.level = .statusBar
    window.collectionBehavior = [.canJoinAllSpaces, .transient]

    let content = NSView(frame: NSRect(x: 0.0, y: 0.0, width: size, height: size))
    content.wantsLayer = true
    window.contentView = content

    let lineWidth = mfxClamp(CGFloat(lineWidthPx), min: 0.5, max: 48.0)
    let alpha = mfxClamp(CGFloat(alphaScale), min: 0.0, max: 1.0)

    for glowPass in 0..<2 {
        let glow = CAShapeLayer()
        glow.frame = content.bounds
        glow.path = path
        glow.fillColor = nil
        glow.strokeColor = mfxColorFromArgb(
            glowArgb,
            alphaScale: alpha * (glowPass == 0 ? 0.38 : 0.22)
        ).cgColor
        glow.lineWidth = lineWidth + CGFloat(10 + glowPass * 5)
        glow.lineJoin = .round
        glow.lineCap = .round
        content.layer?.addSublayer(glow)
    }

    let stroke = CAShapeLayer()
    stroke.frame = content.bounds
    stroke.path = path
    stroke.fillColor = nil
    stroke.strokeColor = mfxColorFromArgb(strokeArgb, alphaScale: alpha).cgColor
    stroke.lineWidth = lineWidth
    stroke.lineJoin = .round
    stroke.lineCap = .round
    content.layer?.addSublayer(stroke)

    let duration = max(0.04, durationSec)
    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = 1.0
    fade.toValue = 0.0
    fade.duration = duration
    fade.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let scale = CABasicAnimation(keyPath: "transform.scale")
    scale.fromValue = 1.0
    scale.toValue = 1.015
    scale.duration = duration
    scale.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let group = CAAnimationGroup()
    group.animations = [fade, scale]
    group.duration = duration
    group.fillMode = .forwards
    group.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxApplyPreferredFrameRate(group, targetFps: targetFps)
    }
    content.layer?.add(group, forKey: "mfx_wasm_polyline_overlay")

    return Unmanaged.passRetained(window).toOpaque()
}

@MainActor
@_cdecl("mfx_macos_wasm_polyline_overlay_create_v1")
public func mfx_macos_wasm_polyline_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ pointsXY: UnsafePointer<Float>?,
    _ pointCount: UInt32,
    _ lineWidthPx: Double,
    _ strokeArgb: UInt32,
    _ glowArgb: UInt32,
    _ alphaScale: Double,
    _ durationSec: Double,
    _ closed: Int32
) -> UnsafeMutableRawPointer? {
    guard let pointsXY else {
        return nil
    }
    return mfxCreateWasmPolylineOverlayOnMainThread(
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        pointsXY: pointsXY,
        pointCount: Int(pointCount),
        lineWidthPx: lineWidthPx,
        strokeArgb: strokeArgb,
        glowArgb: glowArgb,
        alphaScale: alphaScale,
        durationSec: durationSec,
        closed: closed != 0
    )
}

@MainActor
@_cdecl("mfx_macos_wasm_polyline_overlay_show_v1")
public func mfx_macos_wasm_polyline_overlay_show_v1(_ windowHandle: UnsafeMutableRawPointer?) {
    guard let windowHandle else {
        return
    }
    let window = Unmanaged<NSWindow>.fromOpaque(windowHandle).takeUnretainedValue()
    window.orderFrontRegardless()
}
