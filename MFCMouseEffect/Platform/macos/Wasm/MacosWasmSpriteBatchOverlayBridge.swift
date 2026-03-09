@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(_ x: Int32, _ y: Int32) -> Int32

private struct MfxSpriteBatchSpriteRecord {
    var localX: Float
    var localY: Float
    var widthPx: Float
    var heightPx: Float
    var alpha: Float
    var rotationRad: Float
    var tintArgb: UInt32
    var applyTint: UInt32
    var srcU0: Float
    var srcV0: Float
    var srcU1: Float
    var srcV1: Float
    var velocityX: Float
    var velocityY: Float
    var accelerationX: Float
    var accelerationY: Float
    var imagePathUtf8: String
}

private let mfxSpriteBatchSpriteBytes = 64

private func mfxWasmUsesScreenBlend(_ blendMode: UInt32) -> Bool {
    return blendMode == 1 || blendMode == 2
}

private func mfxWasmResolveWindowLevel(sortKey: Int32) -> NSWindow.Level {
    let clamped = max(-256, min(256, Int(sortKey)))
    return NSWindow.Level(rawValue: NSWindow.Level.statusBar.rawValue + clamped)
}

private func mfxSpriteClamp(_ value: CGFloat, min minValue: CGFloat, max maxValue: CGFloat) -> CGFloat {
    if value < minValue {
        return minValue
    }
    if value > maxValue {
        return maxValue
    }
    return value
}

private func mfxSpriteColorFromArgb(_ argb: UInt32, alphaScale: CGFloat) -> NSColor {
    let baseAlpha = CGFloat(Double((argb >> 24) & 0xFF) / 255.0)
    let alpha = mfxSpriteClamp(baseAlpha * alphaScale, min: 0.0, max: 1.0)
    let red = CGFloat(Double((argb >> 16) & 0xFF) / 255.0)
    let green = CGFloat(Double((argb >> 8) & 0xFF) / 255.0)
    let blue = CGFloat(Double(argb & 0xFF) / 255.0)
    return NSColor(calibratedRed: red, green: green, blue: blue, alpha: alpha)
}

private func mfxSpriteCreateTintedImage(_ image: NSImage, tintColor: NSColor) -> NSImage? {
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

private func mfxSpriteResolveOverlayTargetFps(overlayX: Int32, overlayY: Int32) -> Float {
    let intervalMs = Int(mfxOverlayTimerIntervalMs(overlayX, overlayY))
    if intervalMs <= 0 {
        return 60.0
    }
    let fps = max(1, min(240, Int((1000.0 / Double(intervalMs)).rounded())))
    return Float(fps)
}

@available(macOS 12.0, *)
private func mfxSpriteApplyPreferredFrameRate(_ animation: CAAnimation, targetFps: Float) {
    let fps = max(1.0, min(240.0, targetFps))
    animation.preferredFrameRateRange = CAFrameRateRange(
        minimum: fps,
        maximum: fps,
        preferred: fps
    )
}

private func mfxSpriteKeyframeValues(
    _ sprite: MfxSpriteBatchSpriteRecord,
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
        let x = CGFloat(Double(sprite.localX)
            + Double(sprite.velocityX) * time
            + 0.5 * Double(sprite.accelerationX) * time * time)
        let y = CGFloat(Double(sprite.localY)
            + Double(sprite.velocityY) * time
            + 0.5 * Double(sprite.accelerationY) * time * time)
        values.append(NSValue(point: NSPoint(x: x, y: y)))
    }
    return values
}

private func mfxSpriteFallbackLayer(
    tintArgb: UInt32,
    widthPx: CGFloat,
    heightPx: CGFloat,
    alpha: CGFloat
) -> CALayer {
    let sizePx = max(widthPx, heightPx)
    let glowRadius = sizePx * 0.85
    let container = CALayer()
    container.bounds = CGRect(x: 0.0, y: 0.0, width: glowRadius * 2.0, height: glowRadius * 2.0)

    let glow = CALayer()
    glow.bounds = container.bounds
    glow.position = CGPoint(x: glowRadius, y: glowRadius)
    glow.backgroundColor = mfxSpriteColorFromArgb(tintArgb, alphaScale: alpha * 0.24).cgColor
    glow.cornerRadius = glowRadius
    container.addSublayer(glow)

    let core = CALayer()
    core.bounds = CGRect(x: 0.0, y: 0.0, width: widthPx, height: heightPx)
    core.position = CGPoint(x: glowRadius, y: glowRadius)
    core.backgroundColor = mfxSpriteColorFromArgb(tintArgb, alphaScale: alpha).cgColor
    core.cornerRadius = min(widthPx, heightPx) * 0.5
    container.addSublayer(core)

    return container
}

private func mfxSpriteResolvedImage(
    pathUtf8: String,
    tintArgb: UInt32,
    applyTint: Bool,
    cache: inout [String: NSImage]
) -> NSImage? {
    guard !pathUtf8.isEmpty else {
        return nil
    }
    let baseImage: NSImage
    if let cached = cache[pathUtf8] {
        baseImage = cached
    } else {
        guard let loaded = NSImage(contentsOfFile: pathUtf8) else {
            return nil
        }
        cache[pathUtf8] = loaded
        baseImage = loaded
    }
    if !applyTint {
        return baseImage
    }
    return mfxSpriteCreateTintedImage(baseImage, tintColor: mfxSpriteColorFromArgb(tintArgb, alphaScale: 1.0)) ?? baseImage
}

private func mfxSpriteApplyRectClipMask(
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

private func mfxSpriteMakeLayer(
    _ sprite: MfxSpriteBatchSpriteRecord,
    durationSec: Double,
    targetFps: Float,
    imageCache: inout [String: NSImage]
) -> CALayer {
    let widthPx = mfxSpriteClamp(CGFloat(sprite.widthPx), min: 2.0, max: 420.0)
    let heightPx = mfxSpriteClamp(CGFloat(sprite.heightPx), min: 2.0, max: 420.0)
    let alpha = mfxSpriteClamp(CGFloat(sprite.alpha), min: 0.0, max: 1.0)
    let container = CALayer()
    container.bounds = CGRect(x: 0.0, y: 0.0, width: widthPx, height: heightPx)
    container.position = CGPoint(x: CGFloat(sprite.localX), y: CGFloat(sprite.localY))
    container.opacity = Float(alpha)
    if abs(sprite.rotationRad) > 0.001 {
        container.setAffineTransform(CGAffineTransform(rotationAngle: CGFloat(sprite.rotationRad)))
    }

    if let image = mfxSpriteResolvedImage(
        pathUtf8: sprite.imagePathUtf8,
        tintArgb: sprite.tintArgb,
        applyTint: sprite.applyTint != 0,
        cache: &imageCache
    ), let cgImage = image.cgImage(forProposedRect: nil, context: nil, hints: nil) {
        let imageLayer = CALayer()
        imageLayer.bounds = container.bounds
        imageLayer.position = CGPoint(x: widthPx * 0.5, y: heightPx * 0.5)
        imageLayer.contents = cgImage
        let u0 = mfxSpriteClamp(CGFloat(min(sprite.srcU0, sprite.srcU1)), min: 0.0, max: 1.0)
        let v0 = mfxSpriteClamp(CGFloat(min(sprite.srcV0, sprite.srcV1)), min: 0.0, max: 1.0)
        let u1 = mfxSpriteClamp(CGFloat(max(sprite.srcU0, sprite.srcU1)), min: 0.0, max: 1.0)
        let v1 = mfxSpriteClamp(CGFloat(max(sprite.srcV0, sprite.srcV1)), min: 0.0, max: 1.0)
        imageLayer.contentsRect = CGRect(
            x: u0,
            y: v0,
            width: max(0.001, u1 - u0),
            height: max(0.001, v1 - v0)
        )
        imageLayer.contentsGravity = .resize
        imageLayer.magnificationFilter = .trilinear
        imageLayer.minificationFilter = .trilinear
        container.addSublayer(imageLayer)
    } else {
        let fallback = mfxSpriteFallbackLayer(
            tintArgb: sprite.tintArgb,
            widthPx: widthPx * 0.72,
            heightPx: heightPx * 0.72,
            alpha: alpha
        )
        fallback.position = CGPoint(x: widthPx * 0.5, y: heightPx * 0.5)
        container.addSublayer(fallback)
    }

    let sampleCount = max(4, min(16, Int(targetFps / 10.0) + 4))
    let position = CAKeyframeAnimation(keyPath: "position")
    position.values = mfxSpriteKeyframeValues(sprite, durationSec: durationSec, sampleCount: sampleCount)
    position.calculationMode = .linear
    position.duration = durationSec
    position.fillMode = .forwards
    position.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxSpriteApplyPreferredFrameRate(position, targetFps: targetFps)
    }
    container.add(position, forKey: "mfx_wasm_sprite_batch_position")

    let fade = CABasicAnimation(keyPath: "opacity")
    fade.fromValue = alpha
    fade.toValue = 0.0
    fade.duration = durationSec
    fade.timingFunction = CAMediaTimingFunction(name: .easeOut)
    fade.fillMode = .forwards
    fade.isRemovedOnCompletion = false
    if #available(macOS 12.0, *) {
        mfxSpriteApplyPreferredFrameRate(fade, targetFps: targetFps)
    }
    container.add(fade, forKey: "mfx_wasm_sprite_batch_fade")
    return container
}

@MainActor
private func mfxCreateWasmSpriteBatchOverlayOnMainThread(
    frameX: Double,
    frameY: Double,
    frameSize: Double,
    sprites: [MfxSpriteBatchSpriteRecord],
    durationSec: Double,
    blendMode: UInt32,
    sortKey: Int32,
    groupId: UInt32,
    clipLeftPx: Double,
    clipTopPx: Double,
    clipWidthPx: Double,
    clipHeightPx: Double
) -> UnsafeMutableRawPointer? {
    guard !sprites.isEmpty else {
        return nil
    }
    _ = groupId

    let size = max(1.0, CGFloat(frameSize))
    let frame = NSRect(x: frameX, y: frameY, width: size, height: size)
    let overlayX = Int32((frameX + frameSize * 0.5).rounded())
    let overlayY = Int32((frameY + frameSize * 0.5).rounded())
    let targetFps = mfxSpriteResolveOverlayTargetFps(overlayX: overlayX, overlayY: overlayY)

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
    mfxSpriteApplyRectClipMask(
        contentLayer: content.layer,
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        clipLeftPx: clipLeftPx,
        clipTopPx: clipTopPx,
        clipWidthPx: clipWidthPx,
        clipHeightPx: clipHeightPx
    )

    var imageCache: [String: NSImage] = [:]
    let duration = max(0.06, durationSec)
    for sprite in sprites {
        let layer = mfxSpriteMakeLayer(sprite, durationSec: duration, targetFps: targetFps, imageCache: &imageCache)
        content.layer?.addSublayer(layer)
    }

    return Unmanaged.passRetained(window).toOpaque()
}

private func mfxLoadSpriteBatchSprites(
    _ spriteBytes: UnsafeRawPointer,
    count: Int,
    imagePathPtrs: UnsafePointer<UnsafePointer<CChar>?>?
) -> [MfxSpriteBatchSpriteRecord] {
    guard count > 0 else {
        return []
    }

    var sprites: [MfxSpriteBatchSpriteRecord] = []
    sprites.reserveCapacity(count)
    for index in 0..<count {
        let base = spriteBytes.advanced(by: index * mfxSpriteBatchSpriteBytes)
        let path = imagePathPtrs?[index].flatMap { String(cString: $0) } ?? ""
        sprites.append(MfxSpriteBatchSpriteRecord(
            localX: base.load(fromByteOffset: 0, as: Float.self),
            localY: base.load(fromByteOffset: 4, as: Float.self),
            widthPx: base.load(fromByteOffset: 8, as: Float.self),
            heightPx: base.load(fromByteOffset: 12, as: Float.self),
            alpha: base.load(fromByteOffset: 16, as: Float.self),
            rotationRad: base.load(fromByteOffset: 20, as: Float.self),
            tintArgb: base.load(fromByteOffset: 24, as: UInt32.self),
            applyTint: base.load(fromByteOffset: 28, as: UInt32.self),
            srcU0: base.load(fromByteOffset: 32, as: Float.self),
            srcV0: base.load(fromByteOffset: 36, as: Float.self),
            srcU1: base.load(fromByteOffset: 40, as: Float.self),
            srcV1: base.load(fromByteOffset: 44, as: Float.self),
            velocityX: base.load(fromByteOffset: 48, as: Float.self),
            velocityY: base.load(fromByteOffset: 52, as: Float.self),
            accelerationX: base.load(fromByteOffset: 56, as: Float.self),
            accelerationY: base.load(fromByteOffset: 60, as: Float.self),
            imagePathUtf8: path
        ))
    }
    return sprites
}

@MainActor
@_cdecl("mfx_macos_wasm_sprite_batch_overlay_create_v1")
public func mfx_macos_wasm_sprite_batch_overlay_create_v1(
    _ frameX: Double,
    _ frameY: Double,
    _ frameSize: Double,
    _ spriteBytes: UnsafeRawPointer?,
    _ spriteCount: UInt32,
    _ imagePathUtf8Ptrs: UnsafePointer<UnsafePointer<CChar>?>?,
    _ durationSec: Double,
    _ blendMode: UInt32,
    _ sortKey: Int32,
    _ groupId: UInt32,
    _ clipLeftPx: Double,
    _ clipTopPx: Double,
    _ clipWidthPx: Double,
    _ clipHeightPx: Double
) -> UnsafeMutableRawPointer? {
    guard let spriteBytes else {
        return nil
    }
    let count = Int(spriteCount)
    guard count > 0 else {
        return nil
    }
    let sprites = mfxLoadSpriteBatchSprites(spriteBytes, count: count, imagePathPtrs: imagePathUtf8Ptrs)
    return mfxCreateWasmSpriteBatchOverlayOnMainThread(
        frameX: frameX,
        frameY: frameY,
        frameSize: frameSize,
        sprites: sprites,
        durationSec: durationSec,
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
@_cdecl("mfx_macos_wasm_sprite_batch_overlay_show_v1")
public func mfx_macos_wasm_sprite_batch_overlay_show_v1(_ windowHandle: UnsafeMutableRawPointer?) {
    guard let windowHandle else {
        return
    }
    let window = Unmanaged<NSWindow>.fromOpaque(windowHandle).takeUnretainedValue()
    window.orderFrontRegardless()
}
