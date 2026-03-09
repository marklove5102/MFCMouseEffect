@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(
    _ x: Int32,
    _ y: Int32
) -> Int32

private func mfxClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxColorFromArgb(_ argb: UInt32, _ alphaScale: CGFloat) -> NSColor {
    let baseAlpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let alpha = mfxClamp(baseAlpha * alphaScale, min: 0.0, max: 1.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxImageInset(_ size: CGFloat) -> CGFloat {
    let dynamicInset = size * 0.04
    let maxInset = max(0.0, size * 0.35)
    return mfxClamp(dynamicInset, min: 0.0, max: maxInset)
}

private func mfxCreateTintedImage(_ image: NSImage, tintColor: NSColor) -> NSImage? {
    let size = image.size
    guard size.width > 0.0, size.height > 0.0 else {
        return nil
    }
    let rect = NSRect(origin: .zero, size: size)
    let tinted = NSImage(size: size)
    tinted.lockFocus()
    defer { tinted.unlockFocus() }
    image.draw(in: rect, from: .zero, operation: .sourceOver, fraction: 1.0)
    tintColor.set()
    rect.fill(using: .sourceAtop)
    return tinted
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

private func mfxEaseOutCubic(_ t: Double) -> Double {
    let clamped = max(0.0, min(1.0, t))
    return 1.0 - pow(1.0 - clamped, 3.0)
}

@MainActor
private func mfxAnimateWasmWindowMotionOnMainThread(
    window: NSWindow,
    startFrame: NSRect,
    motionDx: Double,
    motionDy: Double,
    durationSec: Double,
    targetFps: Float
) {
    let deltaX = CGFloat(motionDx)
    let deltaY = CGFloat(motionDy)
    if abs(deltaX) <= 0.01, abs(deltaY) <= 0.01 {
        return
    }

    let duration = max(0.01, durationSec)
    let fps = max(1.0, min(240.0, Double(targetFps)))
    let stepSec = max(0.004, min(0.25, 1.0 / fps))
    let startTime = CACurrentMediaTime()
    let startOrigin = startFrame.origin
    let endOrigin = CGPoint(x: startOrigin.x + deltaX, y: startOrigin.y + deltaY)

    let timer = Timer(timeInterval: stepSec, repeats: true) { [weak window] timer in
        guard let window else {
            timer.invalidate()
            return
        }
        let elapsed = max(0.0, CACurrentMediaTime() - startTime)
        let progress = min(1.0, elapsed / duration)
        let eased = mfxEaseOutCubic(progress)
        let x = startOrigin.x + (endOrigin.x - startOrigin.x) * CGFloat(eased)
        let y = startOrigin.y + (endOrigin.y - startOrigin.y) * CGFloat(eased)
        MainActor.assumeIsolated {
            window.setFrameOrigin(NSPoint(x: x, y: y))
        }
        if progress >= 1.0 {
            timer.invalidate()
        }
    }
    RunLoop.main.add(timer, forMode: .common)
}

@MainActor
private func mfxCreateWasmImageOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    imagePathUtf8: String,
    tintArgb: UInt32,
    applyTint: Bool,
    alphaScale: Double,
    durationSec: Double,
    rotationRad: Double,
    motionDx: Double,
    motionDy: Double
) -> UnsafeMutableRawPointer? {
    let overlayX = Int32((frameX + frameSize * 0.5).rounded())
    let overlayY = Int32((frameY + frameSize * 0.5).rounded())
    let targetFps = mfxResolveOverlayTargetFps(overlayX: overlayX, overlayY: overlayY)

    let size = max(1.0, CGFloat(frameSize))
    let frame = NSRect(x: frameX, y: frameY, width: size, height: size)
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

    let alphaScaleClamped = mfxClamp(CGFloat(alphaScale), min: 0.0, max: 1.0)
    var renderedImage = false
    if !imagePathUtf8.isEmpty, let image = NSImage(contentsOfFile: imagePathUtf8) {
        let imageInset = mfxImageInset(size)
        let imageDrawSize = max(1.0, size - imageInset * 2.0)
        let imageOrigin = (size - imageDrawSize) * 0.5
        let resolvedImage: NSImage
        if applyTint {
            // Keep parity with Windows color-matrix semantics:
            // tint alpha is applied once by tint color, then global alpha scales via imageView alphaValue.
            let tintColor = mfxColorFromArgb(tintArgb, 1.0)
            resolvedImage = mfxCreateTintedImage(image, tintColor: tintColor) ?? image
        } else {
            resolvedImage = image
        }
        let imageView = NSImageView(
            frame: NSRect(
                x: imageOrigin,
                y: imageOrigin,
                width: imageDrawSize,
                height: imageDrawSize
            )
        )
        imageView.image = resolvedImage
        imageView.imageScaling = .scaleProportionallyUpOrDown
        imageView.alphaValue = alphaScaleClamped
        content.addSubview(imageView)
        renderedImage = true
        content.layer?.compositingFilter = "screenBlendMode"
    }

    var animationLayer: CALayer? = content.layer
    if !renderedImage {
        let ringInset = mfxClamp(size * 0.13, min: 4.0, max: 36.0)
        let ring = CAShapeLayer()
        ring.frame = content.bounds
        let ringRect = CGRect(
            x: ringInset,
            y: ringInset,
            width: max(1.0, size - ringInset * 2.0),
            height: max(1.0, size - ringInset * 2.0)
        )
        ring.path = CGPath(ellipseIn: ringRect, transform: nil)
        let fillAlphaScale = 0.22 * alphaScaleClamped
        let strokeAlphaScale = 0.95 * alphaScaleClamped
        ring.fillColor = mfxColorFromArgb(tintArgb, fillAlphaScale).cgColor
        ring.strokeColor = mfxColorFromArgb(tintArgb, strokeAlphaScale).cgColor
        ring.lineWidth = mfxClamp(size * 0.022, min: 1.0, max: 5.0)
        ring.opacity = 0.98
        content.layer?.addSublayer(ring)
        animationLayer = ring
    }

    let duration = max(0.05, durationSec)
    let animScale = CABasicAnimation(keyPath: "transform.scale")
    if renderedImage {
        animScale.fromValue = 1.0
        animScale.toValue = 0.55
    } else {
        animScale.fromValue = 0.15
        animScale.toValue = 1.0
    }
    animScale.duration = duration
    animScale.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = 0.98
    fade.toValue = 0.0
    fade.duration = duration
    fade.timingFunction = CAMediaTimingFunction(name: .easeIn)

    let group = CAAnimationGroup()
    group.animations = [animScale, fade]
    group.duration = duration
    group.fillMode = .forwards
    group.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxApplyPreferredFrameRate(group, targetFps: targetFps)
    }
    animationLayer?.add(group, forKey: "mfx_wasm_image_overlay")

    if abs(rotationRad) > 0.001 {
        let rotate = CABasicAnimation(keyPath: "transform.rotation.z")
        rotate.fromValue = 0.0
        rotate.toValue = rotationRad
        rotate.duration = duration
        rotate.timingFunction = CAMediaTimingFunction(name: .easeOut)
        rotate.fillMode = .forwards
        rotate.isRemovedOnCompletion = false
        if #available(macOS 12.0, *) {
            mfxApplyPreferredFrameRate(rotate, targetFps: targetFps)
        }
        content.layer?.add(rotate, forKey: "mfx_wasm_image_rotate")
    }

    mfxAnimateWasmWindowMotionOnMainThread(
        window: window,
        startFrame: frame,
        motionDx: motionDx,
        motionDy: motionDy,
        durationSec: duration,
        targetFps: targetFps
    )

    return Unmanaged.passRetained(window).toOpaque()
}

@MainActor
private func mfxShowWasmImageOverlayOnMainThread(_ windowHandleBits: UInt) {
    guard windowHandleBits != 0 else {
        return
    }
    guard let ptr = UnsafeMutableRawPointer(bitPattern: windowHandleBits) else {
        return
    }
    let window = Unmanaged<NSWindow>.fromOpaque(ptr).takeUnretainedValue()
    window.orderFrontRegardless()
}

@_cdecl("mfx_macos_wasm_image_overlay_create_v1")
public func mfx_macos_wasm_image_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ imagePathUtf8: UnsafePointer<CChar>?,
    _ tintArgb: UInt32,
    _ applyTint: Int32,
    _ alphaScale: Double,
    _ durationSec: Double,
    _ rotationRad: Double,
    _ motionDx: Double,
    _ motionDy: Double
) -> UnsafeMutableRawPointer? {
    let imagePath = imagePathUtf8.map(String.init(cString:)) ?? ""
    let shouldApplyTint = applyTint != 0
    if Thread.isMainThread {
        let bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateWasmImageOverlayOnMainThread(
                    frameX: frameX,
                    frameY: frameY,
                    frameSize: frameSize,
                    imagePathUtf8: imagePath,
                    tintArgb: tintArgb,
                    applyTint: shouldApplyTint,
                    alphaScale: alphaScale,
                    durationSec: durationSec,
                    rotationRad: rotationRad,
                    motionDx: motionDx,
                    motionDy: motionDy
                )
            )
        }
        return UnsafeMutableRawPointer(bitPattern: bits)
    }

    var bits: UInt = 0
    DispatchQueue.main.sync {
        bits = MainActor.assumeIsolated {
            UInt(
                bitPattern: mfxCreateWasmImageOverlayOnMainThread(
                    frameX: frameX,
                    frameY: frameY,
                    frameSize: frameSize,
                    imagePathUtf8: imagePath,
                    tintArgb: tintArgb,
                    applyTint: shouldApplyTint,
                    alphaScale: alphaScale,
                    durationSec: durationSec,
                    rotationRad: rotationRad,
                    motionDx: motionDx,
                    motionDy: motionDy
                )
            )
        }
    }
    return UnsafeMutableRawPointer(bitPattern: bits)
}

@_cdecl("mfx_macos_wasm_image_overlay_show_v1")
public func mfx_macos_wasm_image_overlay_show_v1(_ windowHandle: UnsafeMutableRawPointer?) {
    let windowHandleBits = UInt(bitPattern: windowHandle)
    if windowHandleBits == 0 {
        return
    }
    if Thread.isMainThread {
        MainActor.assumeIsolated {
            mfxShowWasmImageOverlayOnMainThread(windowHandleBits)
        }
        return
    }
    DispatchQueue.main.async {
        MainActor.assumeIsolated {
            mfxShowWasmImageOverlayOnMainThread(windowHandleBits)
        }
    }
}
