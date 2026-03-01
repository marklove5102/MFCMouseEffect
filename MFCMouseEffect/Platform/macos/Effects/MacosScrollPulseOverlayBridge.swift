@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_create_window_v1")
private func mfx_macos_overlay_create_window_v1(
    _ x: Double,
    _ y: Double,
    _ width: Double,
    _ height: Double
) -> UnsafeMutableRawPointer?

@_silgen_name("mfx_macos_overlay_apply_content_scale_v1")
private func mfx_macos_overlay_apply_content_scale_v1(
    _ contentHandle: UnsafeMutableRawPointer?,
    _ x: Int32,
    _ y: Int32
)

private func mfxClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxResolveOpacity(_ baseOpacity: CGFloat, _ delta: CGFloat, _ minOpacity: CGFloat) -> CGFloat {
    let clamped = mfxClamp(baseOpacity + delta, min: 0.0, max: 1.0)
    let floor = mfxClamp(minOpacity, min: 0.0, max: 1.0)
    return max(clamped, floor)
}

private func mfxScaleMetric(
    _ referenceSize: CGFloat,
    _ baseValue: CGFloat,
    _ baseReference: CGFloat,
    _ minValue: CGFloat,
    _ maxValue: CGFloat
) -> CGFloat {
    let safeReference = max(1.0, baseReference)
    let safeSize = max(1.0, referenceSize)
    let scaled = baseValue * (safeSize / safeReference)
    return mfxClamp(scaled, min: minValue, max: maxValue)
}

private func mfxColorFromArgb(_ argb: UInt32) -> NSColor {
    let alpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxCreateScaleFadeAnimationGroup(
    fromScale: CGFloat,
    toScale: CGFloat,
    fromOpacity: CGFloat,
    duration: Double
) -> CAAnimationGroup {
    let clampedDuration = max(0.05, duration)

    let scale = CABasicAnimation(keyPath: "transform.scale")
    scale.fromValue = fromScale
    scale.toValue = toScale
    scale.duration = clampedDuration
    scale.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = mfxClamp(fromOpacity, min: 0.0, max: 1.0)
    fade.toValue = 0.0
    fade.duration = clampedDuration
    fade.timingFunction = CAMediaTimingFunction(name: .easeIn)

    let group = CAAnimationGroup()
    group.animations = [scale, fade]
    group.duration = clampedDuration
    group.fillMode = .forwards
    group.isRemovedOnCompletion = false
    return group
}

private func mfxCreateScrollDirectionArrowPath(bodyRect: CGRect, horizontal: Bool, delta: Int32) -> CGPath {
    let positive = delta >= 0
    let size: CGFloat = 7.0
    let cx: CGFloat
    let cy: CGFloat
    if horizontal {
        cx = positive ? bodyRect.maxX - 9.0 : bodyRect.minX + 9.0
        cy = bodyRect.midY
    } else {
        cx = bodyRect.midX
        cy = positive ? bodyRect.maxY - 9.0 : bodyRect.minY + 9.0
    }

    let path = CGMutablePath()
    if horizontal {
        if positive {
            path.move(to: CGPoint(x: cx + size, y: cy))
            path.addLine(to: CGPoint(x: cx - size * 0.8, y: cy + size * 0.8))
            path.addLine(to: CGPoint(x: cx - size * 0.8, y: cy - size * 0.8))
        } else {
            path.move(to: CGPoint(x: cx - size, y: cy))
            path.addLine(to: CGPoint(x: cx + size * 0.8, y: cy + size * 0.8))
            path.addLine(to: CGPoint(x: cx + size * 0.8, y: cy - size * 0.8))
        }
    } else if positive {
        path.move(to: CGPoint(x: cx, y: cy + size))
        path.addLine(to: CGPoint(x: cx - size * 0.8, y: cy - size * 0.8))
        path.addLine(to: CGPoint(x: cx + size * 0.8, y: cy - size * 0.8))
    } else {
        path.move(to: CGPoint(x: cx, y: cy - size))
        path.addLine(to: CGPoint(x: cx - size * 0.8, y: cy + size * 0.8))
        path.addLine(to: CGPoint(x: cx + size * 0.8, y: cy + size * 0.8))
    }
    path.closeSubpath()
    return path
}

@MainActor
private func mfxCreateScrollPulseOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    bodyX: Double,
    bodyY: Double,
    bodyWidth: Double,
    bodyHeight: Double,
    overlayX: Int32,
    overlayY: Int32,
    horizontal: Bool,
    delta: Int32,
    helixMode: Bool,
    twinkleMode: Bool,
    fillArgb: UInt32,
    strokeArgb: UInt32,
    baseOpacity: Double,
    durationSec: Double
) -> UnsafeMutableRawPointer? {
    let size = max(1.0, frameSize)
    let windowHandle = mfx_macos_overlay_create_window_v1(frameX, frameY, size, size)
    guard let windowHandle else {
        return nil
    }

    let window = Unmanaged<NSWindow>.fromOpaque(windowHandle).takeUnretainedValue()
    guard let content = window.contentView else {
        return windowHandle
    }
    content.wantsLayer = true
    mfx_macos_overlay_apply_content_scale_v1(
        Unmanaged.passUnretained(content).toOpaque(),
        overlayX,
        overlayY
    )

    guard let contentLayer = content.layer else {
        return windowHandle
    }

    let contentBounds = content.bounds
    let bodyRect = CGRect(x: bodyX, y: bodyY, width: bodyWidth, height: bodyHeight)
    let sizeCGFloat = CGFloat(size)
    let baseOpacityCGFloat = CGFloat(baseOpacity)

    let body = CAShapeLayer()
    body.frame = contentBounds
    body.path = CGPath(
        roundedRect: bodyRect,
        cornerWidth: 9.0,
        cornerHeight: 9.0,
        transform: nil
    )
    body.fillColor = mfxColorFromArgb(fillArgb).cgColor
    body.strokeColor = mfxColorFromArgb(strokeArgb).cgColor
    body.lineWidth = 2.0
    body.opacity = Float(mfxResolveOpacity(baseOpacityCGFloat, 0.0, 0.0))
    contentLayer.addSublayer(body)

    let arrow = CAShapeLayer()
    arrow.frame = contentBounds
    arrow.path = mfxCreateScrollDirectionArrowPath(bodyRect: bodyRect, horizontal: horizontal, delta: delta)
    arrow.fillColor = mfxColorFromArgb(strokeArgb).cgColor
    arrow.strokeColor = body.strokeColor
    arrow.opacity = Float(mfxResolveOpacity(baseOpacityCGFloat, 0.02, 0.0))
    contentLayer.addSublayer(arrow)

    if helixMode {
        let helix = CAShapeLayer()
        helix.frame = contentBounds
        let helixExpand = mfxScaleMetric(sizeCGFloat, 9.0, 160.0, 4.0, 18.0)
        let helixRect = bodyRect.insetBy(dx: -helixExpand, dy: -helixExpand)
        helix.path = CGPath(ellipseIn: helixRect, transform: nil)
        helix.fillColor = NSColor.clear.cgColor
        helix.strokeColor = mfxColorFromArgb(strokeArgb).cgColor
        helix.lineWidth = mfxScaleMetric(sizeCGFloat, 1.6, 160.0, 0.8, 3.2)
        helix.opacity = Float(mfxResolveOpacity(baseOpacityCGFloat, -0.14, 0.0))
        contentLayer.addSublayer(helix)

        let spin = CABasicAnimation(keyPath: "transform.rotation")
        spin.fromValue = 0.0
        spin.toValue = Double.pi * 1.5
        spin.duration = mfxClamp(CGFloat(durationSec * 0.55), min: 0.22, max: 0.82)
        spin.repeatCount = 1
        helix.add(spin, forKey: "mfx_scroll_helix_spin")
    }

    if twinkleMode {
        let twinkle = CAShapeLayer()
        twinkle.frame = contentBounds
        let twinkleExpand = mfxScaleMetric(sizeCGFloat, 20.0, 160.0, 8.0, 36.0)
        let twinkleRect = bodyRect.insetBy(dx: -twinkleExpand, dy: -twinkleExpand)
        twinkle.path = CGPath(ellipseIn: twinkleRect, transform: nil)
        twinkle.fillColor = NSColor.clear.cgColor
        twinkle.strokeColor = mfxColorFromArgb(strokeArgb).cgColor
        twinkle.lineWidth = mfxScaleMetric(sizeCGFloat, 1.0, 160.0, 0.8, 2.4)
        twinkle.opacity = Float(mfxResolveOpacity(baseOpacityCGFloat, -0.38, 0.0))
        contentLayer.addSublayer(twinkle)
    }

    let group = mfxCreateScaleFadeAnimationGroup(
        fromScale: 0.72,
        toScale: 1.04,
        fromOpacity: CGFloat(baseOpacity + 0.02),
        duration: durationSec
    )
    body.add(group, forKey: "mfx_scroll_body_pulse")
    arrow.add(group, forKey: "mfx_scroll_arrow_pulse")

    return windowHandle
}

@_cdecl("mfx_macos_scroll_pulse_overlay_create_v1")
public func mfx_macos_scroll_pulse_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ bodyX: Double,
    _ bodyY: Double,
    _ bodyWidth: Double,
    _ bodyHeight: Double,
    _ overlayX: Int32,
    _ overlayY: Int32,
    _ horizontal: Int32,
    _ delta: Int32,
    _ helixMode: Int32,
    _ twinkleMode: Int32,
    _ fillArgb: UInt32,
    _ strokeArgb: UInt32,
    _ baseOpacity: Double,
    _ durationSec: Double
) -> UnsafeMutableRawPointer? {
    let horizontalEnabled = (horizontal != 0)
    let helixEnabled = (helixMode != 0)
    let twinkleEnabled = (twinkleMode != 0)

    if Thread.isMainThread {
        let bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateScrollPulseOverlayOnMainThread(
                    frameX: frameX,
                    frameY: frameY,
                    frameSize: frameSize,
                    bodyX: bodyX,
                    bodyY: bodyY,
                    bodyWidth: bodyWidth,
                    bodyHeight: bodyHeight,
                    overlayX: overlayX,
                    overlayY: overlayY,
                    horizontal: horizontalEnabled,
                    delta: delta,
                    helixMode: helixEnabled,
                    twinkleMode: twinkleEnabled,
                    fillArgb: fillArgb,
                    strokeArgb: strokeArgb,
                    baseOpacity: baseOpacity,
                    durationSec: durationSec
                )
            )
        }
        return UnsafeMutableRawPointer(bitPattern: bits)
    }

    var bits: UInt = 0
    DispatchQueue.main.sync {
        bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateScrollPulseOverlayOnMainThread(
                    frameX: frameX,
                    frameY: frameY,
                    frameSize: frameSize,
                    bodyX: bodyX,
                    bodyY: bodyY,
                    bodyWidth: bodyWidth,
                    bodyHeight: bodyHeight,
                    overlayX: overlayX,
                    overlayY: overlayY,
                    horizontal: horizontalEnabled,
                    delta: delta,
                    helixMode: helixEnabled,
                    twinkleMode: twinkleEnabled,
                    fillArgb: fillArgb,
                    strokeArgb: strokeArgb,
                    baseOpacity: baseOpacity,
                    durationSec: durationSec
                )
            )
        }
    }
    return UnsafeMutableRawPointer(bitPattern: bits)
}

