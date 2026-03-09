@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(_ x: Int32, _ y: Int32) -> Int32

private struct MfxGlowBatchParticleRecord {
    var localX: Float
    var localY: Float
    var radiusPx: Float
    var alpha: Float
    var colorArgb: UInt32
    var velocityX: Float
    var velocityY: Float
    var accelerationX: Float
    var accelerationY: Float
}

private let mfxGlowBatchParticleBytes = 36

private func mfxWasmUsesScreenBlend(_ blendMode: UInt32) -> Bool {
    return blendMode == 1 || blendMode == 2
}

private func mfxWasmResolveWindowLevel(sortKey: Int32) -> NSWindow.Level {
    let clamped = max(-256, min(256, Int(sortKey)))
    return NSWindow.Level(rawValue: NSWindow.Level.statusBar.rawValue + clamped)
}

private func mfxGlowClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxGlowColorFromArgb(_ argb: UInt32, alphaScale: CGFloat) -> NSColor {
    let baseAlpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let alpha = mfxGlowClamp(baseAlpha * alphaScale, min: 0.0, max: 1.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxGlowResolveOverlayTargetFps(overlayX: Int32, overlayY: Int32) -> Float {
    let intervalMs = Int(mfxOverlayTimerIntervalMs(overlayX, overlayY))
    if intervalMs <= 0 {
        return 60.0
    }
    let fps = max(1, min(240, Int((1000.0 / Double(intervalMs)).rounded())))
    return Float(fps)
}

@available(macOS 12.0, *)
private func mfxGlowApplyPreferredFrameRate(_ animation: CAAnimation, targetFps: Float) {
    let fps = max(1.0, min(240.0, targetFps))
    animation.preferredFrameRateRange = CAFrameRateRange(
        minimum: fps,
        maximum: fps,
        preferred: fps
    )
}

private func mfxGlowParticleKeyframeValues(
    _ particle: MfxGlowBatchParticleRecord,
    durationSec: Double,
    sampleCount: Int
) -> [NSValue] {
    let steps = max(2, sampleCount)
    let duration = max(0.01, durationSec)
    var values: [NSValue] = []
    values.reserveCapacity(steps)
    for step in 0..<steps {
        let progress = Double(step) / Double(steps - 1)
        let time = duration * progress
        let x = CGFloat(Double(particle.localX)
            + Double(particle.velocityX) * time
            + 0.5 * Double(particle.accelerationX) * time * time)
        let y = CGFloat(Double(particle.localY)
            + Double(particle.velocityY) * time
            + 0.5 * Double(particle.accelerationY) * time * time)
        values.append(NSValue(point: NSPoint(x: x, y: y)))
    }
    return values
}

private func mfxGlowMakeParticleLayer(
    _ particle: MfxGlowBatchParticleRecord,
    durationSec: Double,
    targetFps: Float
) -> CALayer {
    let radius = mfxGlowClamp(CGFloat(particle.radiusPx), min: 1.0, max: 96.0)
    let alpha = mfxGlowClamp(CGFloat(particle.alpha), min: 0.0, max: 1.0)
    let glowRadius = radius * 3.0
    let container = CALayer()
    container.bounds = CGRect(x: 0.0, y: 0.0, width: glowRadius * 2.0, height: glowRadius * 2.0)
    container.position = CGPoint(x: CGFloat(particle.localX), y: CGFloat(particle.localY))

    let glow = CALayer()
    glow.bounds = CGRect(x: 0.0, y: 0.0, width: glowRadius * 2.0, height: glowRadius * 2.0)
    glow.position = CGPoint(x: glowRadius, y: glowRadius)
    glow.backgroundColor = mfxGlowColorFromArgb(particle.colorArgb, alphaScale: alpha * 0.26).cgColor
    glow.cornerRadius = glowRadius
    glow.opacity = 1.0
    container.addSublayer(glow)

    let core = CALayer()
    core.bounds = CGRect(x: 0.0, y: 0.0, width: radius * 2.0, height: radius * 2.0)
    core.position = CGPoint(x: glowRadius, y: glowRadius)
    core.backgroundColor = mfxGlowColorFromArgb(particle.colorArgb, alphaScale: alpha).cgColor
    core.cornerRadius = radius
    container.addSublayer(core)

    let hotRadius = max(0.8, radius * 0.38)
    let hot = CALayer()
    hot.bounds = CGRect(x: 0.0, y: 0.0, width: hotRadius * 2.0, height: hotRadius * 2.0)
    hot.position = CGPoint(x: glowRadius, y: glowRadius)
    hot.backgroundColor = NSColor(calibratedWhite: 1.0, alpha: alpha * 0.78).cgColor
    hot.cornerRadius = hotRadius
    container.addSublayer(hot)

    let sampleCount = max(4, min(16, Int(targetFps / 10.0) + 4))
    let position = CAKeyframeAnimation(keyPath: "position")
    position.values = mfxGlowParticleKeyframeValues(particle, durationSec: durationSec, sampleCount: sampleCount)
    position.calculationMode = .linear
    position.duration = durationSec
    position.fillMode = .forwards
    position.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxGlowApplyPreferredFrameRate(position, targetFps: targetFps)
    }
    container.add(position, forKey: "mfx_wasm_glow_batch_position")

    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = 1.0
    fade.toValue = 0.0
    fade.duration = durationSec
    fade.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let scale = CABasicAnimation(keyPath: "transform.scale")
    scale.fromValue = 1.0
    scale.toValue = 0.76
    scale.duration = durationSec
    scale.timingFunction = CAMediaTimingFunction(name: .easeOut)

    let group = CAAnimationGroup()
    group.animations = [fade, scale]
    group.duration = durationSec
    group.fillMode = .forwards
    group.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxGlowApplyPreferredFrameRate(group, targetFps: targetFps)
    }
    container.add(group, forKey: "mfx_wasm_glow_batch_fade")

    return container
}

@MainActor
private func mfxCreateWasmGlowBatchOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    particles: [MfxGlowBatchParticleRecord],
    durationSec: Double,
    blendMode: UInt32,
    sortKey: Int32,
    groupId: UInt32
) -> UnsafeMutableRawPointer? {
    guard !particles.isEmpty else {
        return nil
    }
    _ = groupId

    let size = max(1.0, CGFloat(frameSize))
    let frame = NSRect(x: frameX, y: frameY, width: size, height: size)
    let overlayX = Int32((frameX + frameSize * 0.5).rounded())
    let overlayY = Int32((frameY + frameSize * 0.5).rounded())
    let targetFps = mfxGlowResolveOverlayTargetFps(overlayX: overlayX, overlayY: overlayY)

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
    window.level = mfxWasmResolveWindowLevel(sortKey: sortKey)
    window.collectionBehavior = [.canJoinAllSpaces, .transient]

    let content = NSView(frame: NSRect(x: 0.0, y: 0.0, width: size, height: size))
    content.wantsLayer = true
    window.contentView = content
    if mfxWasmUsesScreenBlend(blendMode) {
        content.layer?.compositingFilter = "screenBlendMode"
    } else {
        content.layer?.compositingFilter = nil
    }

    let duration = max(0.06, durationSec)
    for particle in particles {
        let layer = mfxGlowMakeParticleLayer(particle, durationSec: duration, targetFps: targetFps)
        content.layer?.addSublayer(layer)
    }

    return Unmanaged.passRetained(window).toOpaque()
}

private func mfxLoadGlowBatchParticles(
    _ particlesPtr: UnsafeRawPointer,
    count: Int
) -> [MfxGlowBatchParticleRecord] {
    guard count > 0 else {
        return []
    }

    var particles: [MfxGlowBatchParticleRecord] = []
    particles.reserveCapacity(count)
    for index in 0..<count {
        let base = particlesPtr.advanced(by: index * mfxGlowBatchParticleBytes)
        particles.append(MfxGlowBatchParticleRecord(
            localX: base.load(fromByteOffset: 0, as: Float.self),
            localY: base.load(fromByteOffset: 4, as: Float.self),
            radiusPx: base.load(fromByteOffset: 8, as: Float.self),
            alpha: base.load(fromByteOffset: 12, as: Float.self),
            colorArgb: base.load(fromByteOffset: 16, as: UInt32.self),
            velocityX: base.load(fromByteOffset: 20, as: Float.self),
            velocityY: base.load(fromByteOffset: 24, as: Float.self),
            accelerationX: base.load(fromByteOffset: 28, as: Float.self),
            accelerationY: base.load(fromByteOffset: 32, as: Float.self)
        ))
    }
    return particles
}

@MainActor
@_cdecl("mfx_macos_wasm_glow_batch_overlay_create_v1")
public func mfx_macos_wasm_glow_batch_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ particlesPtr: UnsafeRawPointer?,
    _ particleCount: UInt32,
    _ durationSec: Double,
    _ blendMode: UInt32,
    _ sortKey: Int32,
    _ groupId: UInt32
) -> UnsafeMutableRawPointer? {
    guard let particlesPtr else {
        return nil
    }
    let count = Int(particleCount)
    guard count > 0 else {
        return nil
    }
    let particles = mfxLoadGlowBatchParticles(particlesPtr, count: count)
    return mfxCreateWasmGlowBatchOverlayOnMainThread(
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        particles: particles,
        durationSec: durationSec,
        blendMode: blendMode,
        sortKey: sortKey,
        groupId: groupId
    )
}

@MainActor
@_cdecl("mfx_macos_wasm_glow_batch_overlay_show_v1")
public func mfx_macos_wasm_glow_batch_overlay_show_v1(_ windowHandle: UnsafeMutableRawPointer?) {
    guard let windowHandle else {
        return
    }
    let window = Unmanaged<NSWindow>.fromOpaque(windowHandle).takeUnretainedValue()
    window.orderFrontRegardless()
}
