@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(_ x: Int32, _ y: Int32) -> Int32

private let mfxPathNodeBytes = 28

private struct MfxPathNodeRecord {
    var opcode: UInt8
    var x1: Float
    var y1: Float
    var x2: Float
    var y2: Float
    var x3: Float
    var y3: Float
}

private func mfxPathClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxPathColorFromArgb(_ argb: UInt32, alphaScale: CGFloat) -> NSColor {
    let baseAlpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let alpha = mfxPathClamp(baseAlpha * alphaScale, min: 0.0, max: 1.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxPathResolveOverlayTargetFps(overlayX: Int32, overlayY: Int32) -> Float {
    let intervalMs = Int(mfxOverlayTimerIntervalMs(overlayX, overlayY))
    if intervalMs <= 0 {
        return 60.0
    }
    let fps = max(1, min(240, Int((1000.0 / Double(intervalMs)).rounded())))
    return Float(fps)
}

@available(macOS 12.0, *)
private func mfxPathApplyPreferredFrameRate(_ animation: CAAnimation, targetFps: Float) {
    let fps = max(1.0, min(240.0, targetFps))
    animation.preferredFrameRateRange = CAFrameRateRange(
        minimum: fps,
        maximum: fps,
        preferred: fps
    )
}

private func mfxPathUsesScreenBlend(_ blendMode: UInt32) -> Bool {
    return blendMode == 1 || blendMode == 2
}

private func mfxPathResolveWindowLevel(sortKey: Int32) -> NSWindow.Level {
    let clamped = max(-256, min(256, Int(sortKey)))
    return NSWindow.Level.statusBar + clamped
}

private func mfxPathResolveFillRule(_ value: UInt8) -> CAShapeLayerFillRule {
    return value == 1 ? .evenOdd : .nonZero
}

private func mfxPathApplyRectClipMask(
    contentLayer: CALayer?,
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    clipLeftPx: Double,
    clipTopPx: Double,
    clipWidthPx: Double,
    clipHeightPx: Double
) {
    guard let contentLayer else {
        return
    }
    guard clipWidthPx > 0.0 && clipHeightPx > 0.0 else {
        contentLayer.mask = nil
        return
    }

    let bounds = CGRect(x: 0.0, y: 0.0, width: max(1.0, frameSize), height: max(1.0, frameSize))
    let localClip = CGRect(
        x: clipLeftPx - frameX,
        y: clipTopPx - frameY,
        width: clipWidthPx,
        height: clipHeightPx
    )
    let clippedRect = bounds.intersection(localClip)
    let mask = CALayer()
    mask.backgroundColor = NSColor.white.cgColor
    mask.frame = clippedRect.isNull ? .zero : clippedRect
    contentLayer.mask = mask
}

@MainActor
private func mfxLoadPathNodes(
    _ nodesPtr: UnsafeRawPointer,
    count: Int
) -> [MfxPathNodeRecord] {
    guard count > 0 else {
        return []
    }

    var nodes: [MfxPathNodeRecord] = []
    nodes.reserveCapacity(count)
    for index in 0..<count {
        let base = nodesPtr.advanced(by: index * mfxPathNodeBytes)
        nodes.append(MfxPathNodeRecord(
            opcode: base.load(fromByteOffset: 0, as: UInt8.self),
            x1: base.load(fromByteOffset: 4, as: Float.self),
            y1: base.load(fromByteOffset: 8, as: Float.self),
            x2: base.load(fromByteOffset: 12, as: Float.self),
            y2: base.load(fromByteOffset: 16, as: Float.self),
            x3: base.load(fromByteOffset: 20, as: Float.self),
            y3: base.load(fromByteOffset: 24, as: Float.self)
        ))
    }
    return nodes
}

private func mfxBuildPath(nodes: [MfxPathNodeRecord]) -> CGPath? {
    guard !nodes.isEmpty else {
        return nil
    }

    let path = CGMutablePath()
    var haveCurrent = false
    var drewAnySegment = false
    for node in nodes {
        switch node.opcode {
        case 0:
            path.move(to: CGPoint(x: CGFloat(node.x1), y: CGFloat(node.y1)))
            haveCurrent = true
        case 1:
            guard haveCurrent else {
                return nil
            }
            path.addLine(to: CGPoint(x: CGFloat(node.x1), y: CGFloat(node.y1)))
            drewAnySegment = true
        case 2:
            guard haveCurrent else {
                return nil
            }
            path.addQuadCurve(
                to: CGPoint(x: CGFloat(node.x2), y: CGFloat(node.y2)),
                control: CGPoint(x: CGFloat(node.x1), y: CGFloat(node.y1))
            )
            drewAnySegment = true
        case 3:
            guard haveCurrent else {
                return nil
            }
            path.addCurve(
                to: CGPoint(x: CGFloat(node.x3), y: CGFloat(node.y3)),
                control1: CGPoint(x: CGFloat(node.x1), y: CGFloat(node.y1)),
                control2: CGPoint(x: CGFloat(node.x2), y: CGFloat(node.y2))
            )
            drewAnySegment = true
        case 4:
            guard haveCurrent else {
                return nil
            }
            path.closeSubpath()
        default:
            return nil
        }
    }

    return drewAnySegment ? path : nil
}

@MainActor
private func mfxCreateWasmPathFillOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    nodes: [MfxPathNodeRecord],
    glowWidthPx: Double,
    fillArgb: UInt32,
    glowArgb: UInt32,
    alphaScale: Double,
    durationSec: Double,
    fillRule: UInt8,
    blendMode: UInt32,
    sortKey: Int32,
    groupId: UInt32,
    clipLeftPx: Double,
    clipTopPx: Double,
    clipWidthPx: Double,
    clipHeightPx: Double
) -> UnsafeMutableRawPointer? {
    guard let path = mfxBuildPath(nodes: nodes) else {
        return nil
    }
    _ = groupId

    let size = max(1.0, CGFloat(frameSize))
    let frame = NSRect(x: frameX, y: frameY, width: size, height: size)
    let overlayX = Int32((frameX + frameSize * 0.5).rounded())
    let overlayY = Int32((frameY + frameSize * 0.5).rounded())
    let targetFps = mfxPathResolveOverlayTargetFps(overlayX: overlayX, overlayY: overlayY)

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
    window.level = mfxPathResolveWindowLevel(sortKey: sortKey)
    window.collectionBehavior = [.canJoinAllSpaces, .transient]

    let content = NSView(frame: NSRect(x: 0.0, y: 0.0, width: size, height: size))
    content.wantsLayer = true
    window.contentView = content
    if mfxPathUsesScreenBlend(blendMode) {
        content.layer?.compositingFilter = "screenBlendMode"
    } else {
        content.layer?.compositingFilter = nil
    }
    mfxPathApplyRectClipMask(
        contentLayer: content.layer,
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        clipLeftPx: clipLeftPx,
        clipTopPx: clipTopPx,
        clipWidthPx: clipWidthPx,
        clipHeightPx: clipHeightPx
    )

    let resolvedGlowWidth = mfxPathClamp(CGFloat(glowWidthPx), min: 0.0, max: 64.0)
    let resolvedAlpha = mfxPathClamp(CGFloat(alphaScale), min: 0.0, max: 1.0)
    let resolvedFillRule = mfxPathResolveFillRule(fillRule)

    for glowPass in 0..<2 {
        let glow = CAShapeLayer()
        glow.frame = content.bounds
        glow.path = path
        glow.fillColor = nil
        glow.strokeColor = mfxPathColorFromArgb(
            glowArgb,
            alphaScale: resolvedAlpha * (glowPass == 0 ? 0.28 : 0.18)
        ).cgColor
        glow.lineWidth = max(1.0, resolvedGlowWidth + CGFloat(6 + glowPass * 5))
        glow.lineJoin = .round
        glow.lineCap = .round
        glow.fillRule = resolvedFillRule
        content.layer?.addSublayer(glow)
    }

    let fill = CAShapeLayer()
    fill.frame = content.bounds
    fill.path = path
    fill.fillColor = mfxPathColorFromArgb(fillArgb, alphaScale: resolvedAlpha).cgColor
    fill.strokeColor = nil
    fill.fillRule = resolvedFillRule
    content.layer?.addSublayer(fill)

    let duration = max(0.04, durationSec)
    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = 1.0
    fade.toValue = 0.0
    fade.duration = duration
    fade.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let scale = CABasicAnimation(keyPath: "transform.scale")
    scale.fromValue = 1.0
    scale.toValue = 1.02
    scale.duration = duration
    scale.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let group = CAAnimationGroup()
    group.animations = [fade, scale]
    group.duration = duration
    group.fillMode = .forwards
    group.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxPathApplyPreferredFrameRate(group, targetFps: targetFps)
    }
    content.layer?.add(group, forKey: "mfx_wasm_path_fill_overlay")

    return Unmanaged.passRetained(window).toOpaque()
}

@MainActor
@_cdecl("mfx_macos_wasm_path_fill_overlay_create_v1")
public func mfx_macos_wasm_path_fill_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ nodesPtr: UnsafeRawPointer?,
    _ nodeCount: UInt32,
    _ glowWidthPx: Double,
    _ fillArgb: UInt32,
    _ glowArgb: UInt32,
    _ alphaScale: Double,
    _ durationSec: Double,
    _ fillRule: UInt8,
    _ blendMode: UInt32,
    _ sortKey: Int32,
    _ groupId: UInt32,
    _ clipLeftPx: Double,
    _ clipTopPx: Double,
    _ clipWidthPx: Double,
    _ clipHeightPx: Double
) -> UnsafeMutableRawPointer? {
    guard let nodesPtr else {
        return nil
    }
    let nodes = mfxLoadPathNodes(nodesPtr, count: Int(nodeCount))
    return mfxCreateWasmPathFillOverlayOnMainThread(
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        nodes: nodes,
        glowWidthPx: glowWidthPx,
        fillArgb: fillArgb,
        glowArgb: glowArgb,
        alphaScale: alphaScale,
        durationSec: durationSec,
        fillRule: fillRule,
        blendMode: blendMode,
        sortKey: sortKey,
        groupId: groupId,
        clipLeftPx: clipLeftPx,
        clipTopPx: clipTopPx,
        clipWidthPx: clipWidthPx,
        clipHeightPx: clipHeightPx
    )
}

@MainActor
@_cdecl("mfx_macos_wasm_path_fill_overlay_show_v1")
public func mfx_macos_wasm_path_fill_overlay_show_v1(_ windowHandle: UnsafeMutableRawPointer?) {
    guard let windowHandle else {
        return
    }
    let window = Unmanaged<NSWindow>.fromOpaque(windowHandle).takeUnretainedValue()
    window.orderFrontRegardless()
}
