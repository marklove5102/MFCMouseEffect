@preconcurrency import AppKit
@preconcurrency import QuartzCore
@preconcurrency import Foundation

@_silgen_name("mfx_macos_overlay_timer_interval_ms_v1")
private func mfxOverlayTimerIntervalMs(_ x: Int32, _ y: Int32) -> Int32

private func mfxWasmUsesScreenBlend(_ blendMode: UInt32) -> Bool {
    return blendMode == 1 || blendMode == 2
}

private func mfxWasmResolveWindowLevel(sortKey: Int32) -> NSWindow.Level {
    let clamped = max(-256, min(256, Int(sortKey)))
    return NSWindow.Level(rawValue: NSWindow.Level.statusBar.rawValue + clamped)
}

private func mfxWasmClipMaskShapeKind(_ rawShapeKind: UInt32) -> UInt32 {
    return rawShapeKind <= 2 ? rawShapeKind : 0
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

private func mfxRetainedSpriteApplyClipMask(
    layer: CALayer?,
    frameLeftPx: Int32,
    frameTopPx: Int32,
    squareSizePx: Int32,
    clipLeftPx: CGFloat,
    clipTopPx: CGFloat,
    clipWidthPx: CGFloat,
    clipHeightPx: CGFloat,
    maskShapeKind: UInt32,
    cornerRadiusPx: CGFloat
) {
    guard let layer else {
        return
    }
    guard clipWidthPx > 0.0, clipHeightPx > 0.0 else {
        layer.mask = nil
        return
    }

    let bounds = CGRect(
        x: 0.0,
        y: 0.0,
        width: CGFloat(max(1, squareSizePx)),
        height: CGFloat(max(1, squareSizePx))
    )
    let localClip = CGRect(
        x: clipLeftPx - CGFloat(frameLeftPx),
        y: clipTopPx - CGFloat(frameTopPx),
        width: clipWidthPx,
        height: clipHeightPx
    )
    let clippedRect = bounds.intersection(localClip)
    guard !clippedRect.isNull, clippedRect.width > 0.0, clippedRect.height > 0.0 else {
        let mask = CALayer()
        mask.backgroundColor = NSColor.white.cgColor
        mask.frame = .zero
        layer.mask = mask
        return
    }

    let shapeKind = mfxWasmClipMaskShapeKind(maskShapeKind)
    if shapeKind == 0 {
        let mask = CALayer()
        mask.backgroundColor = NSColor.white.cgColor
        mask.frame = clippedRect
        layer.mask = mask
        return
    }

    let shapeMask = CAShapeLayer()
    shapeMask.fillColor = NSColor.white.cgColor
    shapeMask.frame = clippedRect
    let localBounds = CGRect(origin: .zero, size: clippedRect.size)
    if shapeKind == 2 {
        shapeMask.path = CGPath(ellipseIn: localBounds, transform: nil)
    } else {
        let radius = min(max(0.0, cornerRadiusPx), min(clippedRect.width, clippedRect.height) * 0.5)
        shapeMask.path = CGPath(
            roundedRect: localBounds,
            cornerWidth: radius,
            cornerHeight: radius,
            transform: nil)
    }
    layer.mask = shapeMask
}

private final class MfxWasmRetainedSpriteEmitterView: NSView {
    struct Particle {
        var x: CGFloat
        var y: CGFloat
        var sizePx: CGFloat
        var alpha: CGFloat
        var rotationRad: CGFloat
        var tintArgb: UInt32
        var applyTint: Bool
        var imagePathUtf8: String
        var life: CGFloat
    }

    var particles: [Particle] = []
    var imageCache: [String: NSImage] = [:]
    var tintedCache: [String: NSImage] = [:]

    override var isOpaque: Bool {
        return false
    }

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        guard let context = NSGraphicsContext.current?.cgContext else {
            return
        }
        context.clear(bounds)
        for particle in particles {
            let life = max(0.0, min(1.0, particle.life))
            let alpha = max(0.0, min(1.0, particle.alpha * pow(life, 0.62)))
            if alpha <= 0.001 || particle.sizePx <= 0.5 {
                continue
            }
            let drawRect = CGRect(
                x: particle.x - particle.sizePx * 0.5,
                y: particle.y - particle.sizePx * 0.5,
                width: particle.sizePx,
                height: particle.sizePx
            )

            context.saveGState()
            context.translateBy(x: particle.x, y: particle.y)
            if abs(particle.rotationRad) > 0.001 {
                context.rotate(by: particle.rotationRad)
            }
            context.translateBy(x: -particle.x, y: -particle.y)

            if let image = resolvedImage(
                pathUtf8: particle.imagePathUtf8,
                tintArgb: particle.tintArgb,
                applyTint: particle.applyTint
            ) {
                image.draw(in: drawRect, from: .zero, operation: .sourceOver, fraction: alpha)
            } else {
                drawFallback(
                    in: context,
                    rect: drawRect,
                    tintArgb: particle.tintArgb,
                    alpha: alpha
                )
            }
            context.restoreGState()
        }
    }

    private func resolvedImage(pathUtf8: String, tintArgb: UInt32, applyTint: Bool) -> NSImage? {
        guard !pathUtf8.isEmpty else {
            return nil
        }

        let baseImage: NSImage
        if let cached = imageCache[pathUtf8] {
            baseImage = cached
        } else {
            guard let loaded = NSImage(contentsOfFile: pathUtf8) else {
                return nil
            }
            imageCache[pathUtf8] = loaded
            baseImage = loaded
        }
        if !applyTint {
            return baseImage
        }

        let tintKey = "\(pathUtf8)#\(tintArgb)"
        if let cached = tintedCache[tintKey] {
            return cached
        }
        let tinted = mfxSpriteCreateTintedImage(
            baseImage,
            tintColor: mfxSpriteColorFromArgb(tintArgb, alphaScale: 1.0)
        ) ?? baseImage
        tintedCache[tintKey] = tinted
        return tinted
    }

    private func drawFallback(
        in context: CGContext,
        rect: CGRect,
        tintArgb: UInt32,
        alpha: CGFloat
    ) {
        let radius = rect.width * 0.5
        let glowRadius = radius * 1.65
        let glowRect = CGRect(
            x: rect.midX - glowRadius,
            y: rect.midY - glowRadius,
            width: glowRadius * 2.0,
            height: glowRadius * 2.0
        )
        context.setFillColor(mfxSpriteColorFromArgb(tintArgb, alphaScale: alpha * 0.22).cgColor)
        context.fillEllipse(in: glowRect)
        context.setFillColor(mfxSpriteColorFromArgb(tintArgb, alphaScale: alpha).cgColor)
        context.fillEllipse(in: rect)
    }
}

@MainActor
private final class MfxWasmRetainedSpriteEmitterState: NSObject {
    private struct Config {
        var imagePathUtf8: String = ""
        var emissionRatePerSec: CGFloat = 84.0
        var directionRad: CGFloat = 0.0
        var spreadRad: CGFloat = 1.0471976
        var speedMin: CGFloat = 120.0
        var speedMax: CGFloat = 240.0
        var sizeMinPx: CGFloat = 24.0
        var sizeMaxPx: CGFloat = 72.0
        var alphaMin: CGFloat = 0.20
        var alphaMax: CGFloat = 0.92
        var tintArgb: UInt32 = 0xFFFFFFFF
        var applyTint: Bool = false
        var rotationMinRad: CGFloat = -0.35
        var rotationMaxRad: CGFloat = 0.35
        var accelerationX: CGFloat = 0.0
        var accelerationY: CGFloat = 120.0
        var emitterTtlMs: UInt64 = 520
        var particleLifeMs: UInt64 = 920
        var maxParticles: Int = 120
        var blendMode: UInt32 = 0
        var sortKey: Int32 = 0
        var groupId: UInt32 = 0
        var clipLeftPx: CGFloat = 0.0
        var clipTopPx: CGFloat = 0.0
        var clipWidthPx: CGFloat = 0.0
        var clipHeightPx: CGFloat = 0.0
        var clipMaskShapeKind: UInt32 = 0
        var clipCornerRadiusPx: CGFloat = 0.0
    }

    private struct Particle {
        var x: CGFloat
        var y: CGFloat
        var vx: CGFloat
        var vy: CGFloat
        var sizePx: CGFloat
        var alpha: CGFloat
        var rotationRad: CGFloat
        var tintArgb: UInt32
        var applyTint: Bool
        var imagePathUtf8: String
        var ageSec: CGFloat
        var lifeSec: CGFloat
    }

    private var window: NSWindow?
    private var view: MfxWasmRetainedSpriteEmitterView?
    private var timer: DispatchSourceTimer?
    private var timerIntervalMs: Int = 16
    private var frameLeftPx: Int32 = 0
    private var frameTopPx: Int32 = 0
    private var squareSizePx: Int32 = 64
    private var localX: CGFloat = 32.0
    private var localY: CGFloat = 32.0
    private var config = Config()
    private var particles: [Particle] = []
    private var emitAccumulator: CGFloat = 0.0
    private var lastTickMs: UInt64 = 0
    private var emitterExpireTickMs: UInt64 = 0
    private var rngState: UInt32 = 0x9E3779B9
    private var presentationAlphaMultiplier: CGFloat = 1.0
    private var presentationVisible = true
    private var appliedGroupOffsetXPx: CGFloat = 0.0
    private var appliedGroupOffsetYPx: CGFloat = 0.0

    private static func nowMs() -> UInt64 {
        let ms = ProcessInfo.processInfo.systemUptime * 1000.0
        return UInt64(ms.rounded(.down))
    }

    private func nextRandom01() -> CGFloat {
        var x = rngState
        x ^= x << 13
        x ^= x >> 17
        x ^= x << 5
        rngState = x
        return CGFloat(Double(x) / Double(UInt32.max))
    }

    private func random(in range: ClosedRange<CGFloat>) -> CGFloat {
        return range.lowerBound + (range.upperBound - range.lowerBound) * nextRandom01()
    }

    private func resolveTargetFps() -> Int {
        let centerX = frameLeftPx + (squareSizePx / 2)
        let centerY = frameTopPx + (squareSizePx / 2)
        let intervalMs = max(4, min(1000, Int(mfxOverlayTimerIntervalMs(centerX, centerY))))
        return max(1, min(240, Int((1000.0 / Double(intervalMs)).rounded())))
    }

    private func ensureWindowOnMain() {
        let size = CGFloat(max(1, squareSizePx))
        let frame = NSRect(x: Double(frameLeftPx), y: Double(frameTopPx), width: size, height: size)

        if window == nil {
            let createdWindow = NSWindow(
                contentRect: frame,
                styleMask: .borderless,
                backing: .buffered,
                defer: false
            )
            createdWindow.isReleasedWhenClosed = false
            createdWindow.isOpaque = false
            createdWindow.backgroundColor = .clear
            createdWindow.hasShadow = false
            createdWindow.ignoresMouseEvents = true
            createdWindow.collectionBehavior = [.canJoinAllSpaces, .transient]

            let contentView = MfxWasmRetainedSpriteEmitterView(
                frame: NSRect(x: 0.0, y: 0.0, width: size, height: size)
            )
            contentView.wantsLayer = true
            createdWindow.contentView = contentView
            window = createdWindow
            view = contentView
            createdWindow.orderFrontRegardless()
        } else {
            window?.setFrame(frame, display: false)
            view?.frame = NSRect(x: 0.0, y: 0.0, width: size, height: size)
        }

        window?.level = mfxWasmResolveWindowLevel(sortKey: config.sortKey)
        if mfxWasmUsesScreenBlend(config.blendMode) {
            view?.layer?.compositingFilter = "screenBlendMode"
        } else {
            view?.layer?.compositingFilter = nil
        }
        mfxRetainedSpriteApplyClipMask(
            layer: view?.layer,
            frameLeftPx: frameLeftPx,
            frameTopPx: frameTopPx,
            squareSizePx: squareSizePx,
            clipLeftPx: config.clipLeftPx,
            clipTopPx: config.clipTopPx,
            clipWidthPx: config.clipWidthPx,
            clipHeightPx: config.clipHeightPx,
            maskShapeKind: config.clipMaskShapeKind,
            cornerRadiusPx: config.clipCornerRadiusPx
        )
        window?.alphaValue = presentationAlphaMultiplier
        if presentationVisible {
            window?.orderFrontRegardless()
        } else {
            window?.orderOut(nil)
        }
    }

    private func stopTimerOnMain() {
        timer?.setEventHandler {}
        timer?.cancel()
        timer = nil
    }

    private func ensureTimerOnMain() {
        if timer != nil {
            return
        }
        timerIntervalMs = max(4, min(1000, Int((1000.0 / Double(resolveTargetFps())).rounded())))
        let source = DispatchSource.makeTimerSource(queue: DispatchQueue.main)
        source.schedule(deadline: .now(), repeating: .milliseconds(timerIntervalMs))
        source.setEventHandler { [weak self] in
            guard let self else {
                return
            }
            MainActor.assumeIsolated {
                self.tickOnMain()
            }
        }
        timer = source
        source.resume()
    }

    private func spawnParticle() {
        let halfSpread = config.spreadRad * 0.5
        let angle = config.directionRad + random(in: (-halfSpread)...halfSpread)
        let speed = random(in: config.speedMin...config.speedMax)
        let particle = Particle(
            x: localX,
            y: localY,
            vx: cos(angle) * speed,
            vy: sin(angle) * speed,
            sizePx: random(in: config.sizeMinPx...config.sizeMaxPx),
            alpha: random(in: config.alphaMin...config.alphaMax),
            rotationRad: random(in: config.rotationMinRad...config.rotationMaxRad),
            tintArgb: config.tintArgb,
            applyTint: config.applyTint,
            imagePathUtf8: config.imagePathUtf8,
            ageSec: 0.0,
            lifeSec: max(0.06, CGFloat(Double(config.particleLifeMs) / 1000.0))
        )
        particles.append(particle)
    }

    private func updateParticles(dtSec: CGFloat, nowMs: UInt64) {
        if nowMs < emitterExpireTickMs {
            emitAccumulator += config.emissionRatePerSec * dtSec
            var emitCount = Int(floor(emitAccumulator))
            emitAccumulator -= CGFloat(emitCount)
            if particles.count >= config.maxParticles {
                emitCount = 0
            }
            let available = max(0, config.maxParticles - particles.count)
            emitCount = min(emitCount, available)
            if emitCount > 0 {
                particles.reserveCapacity(particles.count + emitCount)
                for _ in 0..<emitCount {
                    spawnParticle()
                }
            }
        }

        var updated: [Particle] = []
        updated.reserveCapacity(particles.count)
        for particle in particles {
            var next = particle
            next.x += next.vx * dtSec + 0.5 * config.accelerationX * dtSec * dtSec
            next.y += next.vy * dtSec + 0.5 * config.accelerationY * dtSec * dtSec
            next.vx += config.accelerationX * dtSec
            next.vy += config.accelerationY * dtSec
            next.ageSec += dtSec
            if next.ageSec < next.lifeSec {
                updated.append(next)
            }
        }
        particles = updated
    }

    private func tickOnMain() {
        guard window != nil else {
            shutdownOnMain()
            return
        }
        let nowMs = Self.nowMs()
        if lastTickMs == 0 {
            lastTickMs = nowMs
        }
        let rawDeltaMs: UInt64 = nowMs >= lastTickMs ? nowMs - lastTickMs : 0
        let deltaMs: UInt64 = max(1, min(100, rawDeltaMs))
        lastTickMs = nowMs
        let dtSec = CGFloat(Double(deltaMs) / 1000.0)

        updateParticles(dtSec: dtSec, nowMs: nowMs)

        if nowMs >= emitterExpireTickMs && particles.isEmpty {
            shutdownOnMain()
            return
        }

        view?.particles = particles.map { particle in
            let life = max(0.0, 1.0 - particle.ageSec / max(0.01, particle.lifeSec))
            return MfxWasmRetainedSpriteEmitterView.Particle(
                x: particle.x,
                y: particle.y,
                sizePx: particle.sizePx,
                alpha: particle.alpha,
                rotationRad: particle.rotationRad,
                tintArgb: particle.tintArgb,
                applyTint: particle.applyTint,
                imagePathUtf8: particle.imagePathUtf8,
                life: life
            )
        }
        view?.needsDisplay = true
    }

    func upsert(
        frameLeftPx: Int32,
        frameTopPx: Int32,
        squareSizePx: Int32,
        localX: CGFloat,
        localY: CGFloat,
        imagePathUtf8: String,
        emissionRatePerSec: CGFloat,
        directionRad: CGFloat,
        spreadRad: CGFloat,
        speedMin: CGFloat,
        speedMax: CGFloat,
        sizeMinPx: CGFloat,
        sizeMaxPx: CGFloat,
        alphaMin: CGFloat,
        alphaMax: CGFloat,
        tintArgb: UInt32,
        applyTint: Bool,
        rotationMinRad: CGFloat,
        rotationMaxRad: CGFloat,
        accelerationX: CGFloat,
        accelerationY: CGFloat,
        emitterTtlMs: UInt32,
        particleLifeMs: UInt32,
        maxParticles: UInt16,
        blendMode: UInt32,
        sortKey: Int32,
        groupId: UInt32,
        clipLeftPx: CGFloat,
        clipTopPx: CGFloat,
        clipWidthPx: CGFloat,
        clipHeightPx: CGFloat
    ) {
        self.frameLeftPx = frameLeftPx
        self.frameTopPx = frameTopPx
        if appliedGroupOffsetXPx != 0.0 || appliedGroupOffsetYPx != 0.0 {
            self.frameLeftPx += Int32(appliedGroupOffsetXPx.rounded())
            self.frameTopPx += Int32(appliedGroupOffsetYPx.rounded())
        }
        self.squareSizePx = max(1, squareSizePx)
        self.localX = localX
        self.localY = localY
        config.imagePathUtf8 = imagePathUtf8
        config.emissionRatePerSec = max(1.0, emissionRatePerSec)
        config.directionRad = directionRad
        config.spreadRad = max(0.0, spreadRad)
        config.speedMin = max(1.0, speedMin)
        config.speedMax = max(config.speedMin, speedMax)
        config.sizeMinPx = max(4.0, sizeMinPx)
        config.sizeMaxPx = max(config.sizeMinPx, sizeMaxPx)
        config.alphaMin = mfxSpriteClamp(alphaMin, min: 0.01, max: 1.0)
        config.alphaMax = mfxSpriteClamp(alphaMax, min: config.alphaMin, max: 1.0)
        config.tintArgb = tintArgb
        config.applyTint = applyTint
        config.rotationMinRad = rotationMinRad
        config.rotationMaxRad = max(rotationMinRad, rotationMaxRad)
        config.accelerationX = accelerationX
        config.accelerationY = accelerationY
        config.emitterTtlMs = UInt64(max(40, min(10000, Int(emitterTtlMs))))
        config.particleLifeMs = UInt64(max(60, min(12000, Int(particleLifeMs))))
        config.maxParticles = max(1, min(384, Int(maxParticles)))
        config.blendMode = blendMode
        config.sortKey = sortKey
        config.groupId = groupId
        config.clipLeftPx = clipLeftPx
        config.clipTopPx = clipTopPx
        config.clipWidthPx = max(0.0, clipWidthPx)
        config.clipHeightPx = max(0.0, clipHeightPx)
        emitterExpireTickMs = Self.nowMs() + config.emitterTtlMs
        if rngState == 0 {
            rngState = 0x9E3779B9
        }

        ensureWindowOnMain()
        ensureTimerOnMain()
    }

    func isActive() -> Bool {
        if window == nil {
            return false
        }
        if timer == nil {
            return !particles.isEmpty
        }
        return true
    }

    func shutdownOnMain() {
        stopTimerOnMain()
        view?.particles = []
        view?.needsDisplay = true
        window?.orderOut(nil)
        window?.close()
        window = nil
        view = nil
        particles.removeAll(keepingCapacity: false)
        emitAccumulator = 0.0
        lastTickMs = 0
        emitterExpireTickMs = 0
    }

    func setGroupPresentationOnMain(alphaMultiplier: CGFloat, visible: Bool) {
        presentationAlphaMultiplier = mfxSpriteClamp(alphaMultiplier, min: 0.0, max: 1.0)
        presentationVisible = visible
        window?.alphaValue = presentationAlphaMultiplier
        if visible {
            window?.orderFrontRegardless()
        } else {
            window?.orderOut(nil)
        }
    }

    func setEffectiveClipRectOnMain(
        clipLeftPx: CGFloat,
        clipTopPx: CGFloat,
        clipWidthPx: CGFloat,
        clipHeightPx: CGFloat,
        maskShapeKind: UInt32 = 0,
        cornerRadiusPx: CGFloat = 0.0
    ) {
        config.clipLeftPx = clipLeftPx
        config.clipTopPx = clipTopPx
        config.clipWidthPx = max(0.0, clipWidthPx)
        config.clipHeightPx = max(0.0, clipHeightPx)
        config.clipMaskShapeKind = mfxWasmClipMaskShapeKind(maskShapeKind)
        config.clipCornerRadiusPx = max(0.0, cornerRadiusPx)
        mfxRetainedSpriteApplyClipMask(
            layer: view?.layer,
            frameLeftPx: frameLeftPx,
            frameTopPx: frameTopPx,
            squareSizePx: squareSizePx,
            clipLeftPx: config.clipLeftPx,
            clipTopPx: config.clipTopPx,
            clipWidthPx: config.clipWidthPx,
            clipHeightPx: config.clipHeightPx,
            maskShapeKind: config.clipMaskShapeKind,
            cornerRadiusPx: config.clipCornerRadiusPx
        )
        view?.needsDisplay = true
    }

    func setEffectiveLayerOnMain(blendMode: UInt32, sortKey: Int32) {
        config.blendMode = blendMode
        config.sortKey = sortKey
        window?.level = mfxWasmResolveWindowLevel(sortKey: sortKey)
        view?.layer?.compositingFilter = mfxWasmUsesScreenBlend(blendMode) ? "screenBlendMode" : nil
    }

    func setEffectiveTranslationOnMain(offsetXPx: CGFloat, offsetYPx: CGFloat) {
        let deltaX = offsetXPx - appliedGroupOffsetXPx
        let deltaY = offsetYPx - appliedGroupOffsetYPx
        appliedGroupOffsetXPx = offsetXPx
        appliedGroupOffsetYPx = offsetYPx
        if deltaX == 0.0 && deltaY == 0.0 {
            return
        }
        frameLeftPx += Int32(deltaX.rounded())
        frameTopPx += Int32(deltaY.rounded())
        ensureWindowOnMain()
        view?.needsDisplay = true
    }
}

private func mfxRetainedSpriteEmitterState(from handle: UnsafeMutableRawPointer?) -> MfxWasmRetainedSpriteEmitterState? {
    let bits = UInt(bitPattern: handle)
    if bits == 0 {
        return nil
    }
    guard let ptr = UnsafeMutableRawPointer(bitPattern: bits) else {
        return nil
    }
    return Unmanaged<MfxWasmRetainedSpriteEmitterState>.fromOpaque(ptr).takeUnretainedValue()
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_create_v1")
public func mfx_macos_wasm_retained_sprite_emitter_create_v1() -> UnsafeMutableRawPointer? {
    if Thread.isMainThread {
        let bits = MainActor.assumeIsolated {
            UInt(bitPattern: Unmanaged.passRetained(MfxWasmRetainedSpriteEmitterState()).toOpaque())
        }
        return UnsafeMutableRawPointer(bitPattern: bits)
    }
    var bits: UInt = 0
    DispatchQueue.main.sync {
        bits = MainActor.assumeIsolated {
            UInt(bitPattern: Unmanaged.passRetained(MfxWasmRetainedSpriteEmitterState()).toOpaque())
        }
    }
    return UnsafeMutableRawPointer(bitPattern: bits)
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_release_v1")
public func mfx_macos_wasm_retained_sprite_emitter_release_v1(_ handle: UnsafeMutableRawPointer?) {
    let bits = UInt(bitPattern: handle)
    if bits == 0 {
        return
    }
    guard let ptr = UnsafeMutableRawPointer(bitPattern: bits) else {
        return
    }
    let state = Unmanaged<MfxWasmRetainedSpriteEmitterState>.fromOpaque(ptr).takeRetainedValue()
    let releaseBlock = {
        MainActor.assumeIsolated {
            state.shutdownOnMain()
        }
    }
    if Thread.isMainThread {
        releaseBlock()
    } else {
        DispatchQueue.main.sync(execute: releaseBlock)
    }
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_upsert_v1")
public func mfx_macos_wasm_retained_sprite_emitter_upsert_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ frameLeftPx: Int32,
    _ frameTopPx: Int32,
    _ squareSizePx: Int32,
    _ localX: Float,
    _ localY: Float,
    _ imagePathUtf8: UnsafePointer<CChar>?,
    _ emissionRatePerSec: Float,
    _ directionRad: Float,
    _ spreadRad: Float,
    _ speedMin: Float,
    _ speedMax: Float,
    _ sizeMinPx: Float,
    _ sizeMaxPx: Float,
    _ alphaMin: Float,
    _ alphaMax: Float,
    _ tintArgb: UInt32,
    _ applyTint: UInt32,
    _ rotationMinRad: Float,
    _ rotationMaxRad: Float,
    _ accelerationX: Float,
    _ accelerationY: Float,
    _ emitterTtlMs: UInt32,
    _ particleLifeMs: UInt32,
    _ maxParticles: UInt16,
    _ blendMode: UInt32,
    _ sortKey: Int32,
    _ groupId: UInt32,
    _ clipLeftPx: Float,
    _ clipTopPx: Float,
    _ clipWidthPx: Float,
    _ clipHeightPx: Float
) {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return
    }
    let imagePath = imagePathUtf8 != nil ? String(cString: imagePathUtf8!) : ""
    let updateBlock = {
        MainActor.assumeIsolated {
            state.upsert(
                frameLeftPx: frameLeftPx,
                frameTopPx: frameTopPx,
                squareSizePx: squareSizePx,
                localX: CGFloat(localX),
                localY: CGFloat(localY),
                imagePathUtf8: imagePath,
                emissionRatePerSec: CGFloat(emissionRatePerSec),
                directionRad: CGFloat(directionRad),
                spreadRad: CGFloat(spreadRad),
                speedMin: CGFloat(speedMin),
                speedMax: CGFloat(speedMax),
                sizeMinPx: CGFloat(sizeMinPx),
                sizeMaxPx: CGFloat(sizeMaxPx),
                alphaMin: CGFloat(alphaMin),
                alphaMax: CGFloat(alphaMax),
                tintArgb: tintArgb,
                applyTint: applyTint != 0,
                rotationMinRad: CGFloat(rotationMinRad),
                rotationMaxRad: CGFloat(rotationMaxRad),
                accelerationX: CGFloat(accelerationX),
                accelerationY: CGFloat(accelerationY),
                emitterTtlMs: emitterTtlMs,
                particleLifeMs: particleLifeMs,
                maxParticles: maxParticles,
                blendMode: blendMode,
                sortKey: sortKey,
                groupId: groupId,
                clipLeftPx: CGFloat(clipLeftPx),
                clipTopPx: CGFloat(clipTopPx),
                clipWidthPx: CGFloat(clipWidthPx),
                clipHeightPx: CGFloat(clipHeightPx)
            )
        }
    }
    if Thread.isMainThread {
        updateBlock()
    } else {
        DispatchQueue.main.async {
            updateBlock()
        }
    }
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_is_active_v1")
public func mfx_macos_wasm_retained_sprite_emitter_is_active_v1(_ handle: UnsafeMutableRawPointer?) -> Int32 {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return 0
    }
    if Thread.isMainThread {
        return MainActor.assumeIsolated {
            state.isActive() ? 1 : 0
        }
    }
    var active: Int32 = 0
    DispatchQueue.main.sync {
        active = MainActor.assumeIsolated {
            state.isActive() ? 1 : 0
        }
    }
    return active
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_set_group_presentation_v1")
public func mfx_macos_wasm_retained_sprite_emitter_set_group_presentation_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ alphaMultiplier: Float,
    _ visible: UInt32
) {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return
    }
    let updateBlock = {
        MainActor.assumeIsolated {
            state.setGroupPresentationOnMain(
                alphaMultiplier: CGFloat(alphaMultiplier),
                visible: visible != 0
            )
        }
    }
    if Thread.isMainThread {
        updateBlock()
    } else {
        DispatchQueue.main.async {
            updateBlock()
        }
    }
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v1")
public func mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ clipLeftPx: Float,
    _ clipTopPx: Float,
    _ clipWidthPx: Float,
    _ clipHeightPx: Float
) {
    mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v2(
        handle,
        clipLeftPx,
        clipTopPx,
        clipWidthPx,
        clipHeightPx,
        0,
        0.0)
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v2")
public func mfx_macos_wasm_retained_sprite_emitter_set_effective_clip_rect_v2(
    _ handle: UnsafeMutableRawPointer?,
    _ clipLeftPx: Float,
    _ clipTopPx: Float,
    _ clipWidthPx: Float,
    _ clipHeightPx: Float,
    _ maskShapeKind: UInt32,
    _ cornerRadiusPx: Float
) {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return
    }
    let updateBlock = {
        MainActor.assumeIsolated {
            state.setEffectiveClipRectOnMain(
                clipLeftPx: CGFloat(clipLeftPx),
                clipTopPx: CGFloat(clipTopPx),
                clipWidthPx: CGFloat(clipWidthPx),
                clipHeightPx: CGFloat(clipHeightPx),
                maskShapeKind: maskShapeKind,
                cornerRadiusPx: CGFloat(cornerRadiusPx)
            )
        }
    }
    if Thread.isMainThread {
        updateBlock()
    } else {
        DispatchQueue.main.async {
            updateBlock()
        }
    }
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_set_effective_layer_v1")
public func mfx_macos_wasm_retained_sprite_emitter_set_effective_layer_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ blendMode: UInt32,
    _ sortKey: Int32
) {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return
    }
    let updateBlock = {
        MainActor.assumeIsolated {
            state.setEffectiveLayerOnMain(blendMode: blendMode, sortKey: sortKey)
        }
    }
    if Thread.isMainThread {
        updateBlock()
    } else {
        DispatchQueue.main.async {
            updateBlock()
        }
    }
}

@_cdecl("mfx_macos_wasm_retained_sprite_emitter_set_effective_translation_v1")
public func mfx_macos_wasm_retained_sprite_emitter_set_effective_translation_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ offsetXPx: Float,
    _ offsetYPx: Float
) {
    guard let state = mfxRetainedSpriteEmitterState(from: handle) else {
        return
    }
    let updateBlock = {
        MainActor.assumeIsolated {
            state.setEffectiveTranslationOnMain(
                offsetXPx: CGFloat(offsetXPx),
                offsetYPx: CGFloat(offsetYPx)
            )
        }
    }
    if Thread.isMainThread {
        updateBlock()
    } else {
        DispatchQueue.main.async {
            updateBlock()
        }
    }
}
