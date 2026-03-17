@preconcurrency import AppKit
@preconcurrency import Foundation
@preconcurrency import QuartzCore
@preconcurrency import SceneKit
import simd

private enum MfxEdgeClampMode: Int32 {
    case strict = 0
    case soft = 1
    case free = 2
}

@MainActor
private final class MfxMouseCompanionHost {
    private let window: NSWindow
    private let sceneView: SCNView
    private let rootNode = SCNNode()
    private let cameraNode = SCNNode()
    private var modelNode: SCNNode?
    private var boneNodesByName: [String: [SCNNode]] = [:]
    private var accessoryNodesByName: [String: [SCNNode]] = [:]
    private var poseBindingNodesByIndex: [[SCNNode]] = []
    private var restLocalTransformByNode: [ObjectIdentifier: simd_float4x4] = [:]
    private var materialEntriesByName: [String: [SCNMaterial]] = [:]
    private var originalDiffuseByMaterial: [ObjectIdentifier: Any] = [:]
    private var knownAccessoryNames: Set<String> = []
    private var activeSkinVariantId: String = ""
    private var baseScale: CGFloat = 1.0
    private var modelBasePosition = SCNVector3Zero
    private let modelFacingYaw: CGFloat = .pi
    private var lastActionCode: Int32 = -1
    private var followOffsetX: CGFloat = 18.0
    private var followOffsetY: CGFloat = 26.0
    private var pressLiftPx: CGFloat = 24.0
    private var edgeClampMode: MfxEdgeClampMode = .soft

    init(sizePx: Int) {
        // Keep generous canvas padding so different model proportions are not clipped.
        let side = CGFloat(max(240, sizePx + 128))
        let windowFrame = NSRect(x: 240.0, y: 220.0, width: side, height: side)
        let contentBounds = NSRect(x: 0.0, y: 0.0, width: side, height: side)

        window = NSWindow(
            contentRect: windowFrame,
            styleMask: [.borderless],
            backing: .buffered,
            defer: false)
        window.isOpaque = false
        window.backgroundColor = .clear
        window.hasShadow = false
        window.level = .statusBar
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary, .stationary]
        window.ignoresMouseEvents = true

        sceneView = SCNView(frame: contentBounds)
        sceneView.wantsLayer = true
        sceneView.backgroundColor = .clear
        sceneView.allowsCameraControl = false
        sceneView.rendersContinuously = true
        sceneView.antialiasingMode = .multisampling4X
        sceneView.autoenablesDefaultLighting = false
        sceneView.autoresizingMask = [.width, .height]

        let scene = SCNScene()
        scene.rootNode.addChildNode(rootNode)
        sceneView.scene = scene
        sceneView.isPlaying = true

        cameraNode.camera = SCNCamera()
        cameraNode.camera?.zNear = 0.01
        cameraNode.camera?.zFar = 1000.0
        cameraNode.camera?.fieldOfView = 62.0
        cameraNode.position = SCNVector3(x: 0.0, y: 0.30, z: 2.0)
        scene.rootNode.addChildNode(cameraNode)

        let keyLight = SCNNode()
        keyLight.light = SCNLight()
        keyLight.light?.type = .directional
        keyLight.light?.intensity = 620
        keyLight.light?.castsShadow = false
        keyLight.eulerAngles = SCNVector3(x: -0.78, y: 0.95, z: 0.0)
        scene.rootNode.addChildNode(keyLight)

        let fillLight = SCNNode()
        fillLight.light = SCNLight()
        fillLight.light?.type = .ambient
        fillLight.light?.intensity = 760
        scene.rootNode.addChildNode(fillLight)

        let rimLight = SCNNode()
        rimLight.light = SCNLight()
        rimLight.light?.type = .directional
        rimLight.light?.intensity = 170
        rimLight.light?.castsShadow = false
        rimLight.eulerAngles = SCNVector3(x: -0.45, y: -2.35, z: 0.0)
        scene.rootNode.addChildNode(rimLight)

        let hostView = NSView(frame: contentBounds)
        hostView.wantsLayer = true
        hostView.layer?.backgroundColor = NSColor.clear.cgColor
        hostView.addSubview(sceneView)
        window.contentView = hostView
    }

    func show() {
        if let screen = NSScreen.main {
            let visible = screen.visibleFrame
            let centered = NSPoint(
                x: visible.midX - window.frame.width * 0.5,
                y: visible.midY - window.frame.height * 0.5)
            window.setFrameOrigin(clampToVisibleDesktop(centered))
        }
        window.orderFrontRegardless()
    }

    func hide() {
        window.orderOut(nil)
    }

    func loadModel(path: String) -> Bool {
        let url = URL(fileURLWithPath: path)
        guard FileManager.default.fileExists(atPath: url.path) else {
            return false
        }
        guard let scene = loadScene(url: url) else {
            return false
        }
        guard !scene.rootNode.childNodes.isEmpty else {
            return false
        }

        let container = SCNNode()
        for child in scene.rootNode.childNodes {
            container.addChildNode(child.clone())
        }
        guard containsRenderableGeometry(root: container) else {
            return false
        }

        modelNode?.removeFromParentNode()
        rootNode.addChildNode(container)
        modelNode = container
        rebuildBoneNodeIndex(root: container)
        rebuildAccessoryNodeIndex(root: container)
        rebuildMaterialIndex(root: container)
        poseBindingNodesByIndex.removeAll(keepingCapacity: true)
        normalizeModelTransform()
        return true
    }

    func update(cursorX: Int32, cursorY: Int32, actionCode: Int32, actionIntensity: Float, skeletonBoneCount: Int32) {
        let cursor = NSPoint(x: CGFloat(cursorX), y: CGFloat(cursorY))
        let dynamicFollowOffsetX = CGFloat(min(22, max(0, Int(actionIntensity * 18.0))))
        let activeIntensity = CGFloat(max(0.0, min(1.0, actionIntensity)))
        let activePressLift = (actionCode == 2 || actionCode == 3) ? (pressLiftPx * activeIntensity) : 0.0
        // Anchor around companion center instead of left edge, so edge-follow feels symmetric.
        let unclampedOrigin = NSPoint(
            x: cursor.x - window.frame.width * 0.5 + followOffsetX + dynamicFollowOffsetX,
            y: cursor.y - window.frame.height * 0.52 + followOffsetY + activePressLift)
        let targetOrigin = clampToVisibleDesktop(unclampedOrigin)
        window.setFrameOrigin(targetOrigin)

        guard let modelNode else {
            return
        }

        let tilt = CGFloat(max(-1.0, min(1.0, actionIntensity)))
        modelNode.eulerAngles.x = -0.08 + tilt * 0.06
        modelNode.eulerAngles.y = modelFacingYaw + ((actionCode == 3) ? -0.20 : 0.0)

        if actionCode != lastActionCode {
            lastActionCode = actionCode
            runActionPulse(node: modelNode, actionCode: actionCode, intensity: actionIntensity)
        }

        let sway = sin(CACurrentMediaTime() * 1.8) * Double(0.010 + min(0.035, skeletonBoneCount > 0 ? 0.012 : 0.0))
        modelNode.position.x = modelBasePosition.x + CGFloat(sway)
        modelNode.position.y = modelBasePosition.y
        modelNode.position.z = modelBasePosition.z
    }

    func configureFollowProfile(
        offsetX: Int32,
        offsetY: Int32,
        pressLiftPx: Int32,
        edgeClampModeCode: Int32
    ) {
        followOffsetX = CGFloat(offsetX)
        followOffsetY = CGFloat(offsetY)
        self.pressLiftPx = CGFloat(max(0, pressLiftPx))
        edgeClampMode = MfxEdgeClampMode(rawValue: edgeClampModeCode) ?? .soft
    }

    private func clampToVisibleDesktop(_ desiredOrigin: NSPoint) -> NSPoint {
        if edgeClampMode == .free {
            return desiredOrigin
        }
        let screens = NSScreen.screens
        guard !screens.isEmpty else {
            return desiredOrigin
        }

        // Use full screen frame (not visibleFrame) so Dock/menu bar are not treated as hard stop bounds.
        var desktopBounds = screens[0].frame
        if screens.count > 1 {
            for screen in screens.dropFirst() {
                desktopBounds = desktopBounds.union(screen.frame)
            }
        }

        let width = window.frame.width
        let height = window.frame.height
        if edgeClampMode == .strict {
            let minX = desktopBounds.minX
            let minY = desktopBounds.minY
            let maxX = desktopBounds.maxX - width
            let maxY = desktopBounds.maxY - height
            let upperX = max(maxX, minX)
            let upperY = max(maxY, minY)
            let clampedX = min(max(desiredOrigin.x, minX), upperX)
            let clampedY = min(max(desiredOrigin.y, minY), upperY)
            return NSPoint(x: clampedX, y: clampedY)
        }

        // Wide soft-edge policy:
        // allow most of the companion window to move outside screen bounds so
        // edge-follow does not saturate early when cursor keeps moving outward.
        let minOverflowX = width * 0.90 + abs(followOffsetX)
        let minOverflowY = height * 0.90 + abs(followOffsetY)
        let overflowX = max(minOverflowX, width * 1.25)
        let overflowY = max(minOverflowY, height * 1.25)

        let minX = desktopBounds.minX - overflowX
        let minY = desktopBounds.minY - overflowY
        let maxX = desktopBounds.maxX - width + overflowX
        let maxY = desktopBounds.maxY - height + overflowY
        let upperX = max(maxX, minX)
        let upperY = max(maxY, minY)
        let clampedX = min(max(desiredOrigin.x, minX), upperX)
        let clampedY = min(max(desiredOrigin.y, minY), upperY)
        return NSPoint(x: clampedX, y: clampedY)
    }

    func configurePoseBinding(names: [String]) -> Bool {
        guard modelNode != nil, !names.isEmpty else {
            poseBindingNodesByIndex.removeAll(keepingCapacity: true)
            return false
        }
        var mapping: [[SCNNode]] = []
        mapping.reserveCapacity(names.count)
        for name in names {
            let key = Self.normalizedBoneName(name)
            if let key, let nodes = boneNodesByName[key], !nodes.isEmpty {
                mapping.append(nodes)
            } else {
                mapping.append([])
            }
        }
        poseBindingNodesByIndex = mapping
        return !poseBindingNodesByIndex.isEmpty
    }

    func applyPose(indices: [Int32], positions: [Float], rotations: [Float], scales: [Float]) {
        guard modelNode != nil, !indices.isEmpty, !poseBindingNodesByIndex.isEmpty else {
            return
        }
        let poseCount = indices.count
        if positions.count < poseCount * 3 || rotations.count < poseCount * 4 || scales.count < poseCount * 3 {
            return
        }

        // Reset all mapped bones every frame so missing bones fall back to rest pose.
        for nodes in poseBindingNodesByIndex {
            for node in nodes {
                let nodeId = ObjectIdentifier(node)
                guard let restLocal = restLocalTransformByNode[nodeId] else {
                    continue
                }
                node.simdTransform = restLocal
            }
        }

        for idx in 0..<poseCount {
            let poseBoneIndex = Int(indices[idx])
            if poseBoneIndex < 0 || poseBoneIndex >= poseBindingNodesByIndex.count {
                continue
            }
            let nodes = poseBindingNodesByIndex[poseBoneIndex]
            if nodes.isEmpty {
                continue
            }
            let pBase = idx * 3
            let rBase = idx * 4
            let localDelta = composeLocalDelta(
                positionX: positions[pBase],
                positionY: positions[pBase + 1],
                positionZ: positions[pBase + 2],
                rotationX: rotations[rBase],
                rotationY: rotations[rBase + 1],
                rotationZ: rotations[rBase + 2],
                rotationW: rotations[rBase + 3],
                scaleX: scales[pBase],
                scaleY: scales[pBase + 1],
                scaleZ: scales[pBase + 2])
            for node in nodes {
                let nodeId = ObjectIdentifier(node)
                guard let restLocal = restLocalTransformByNode[nodeId] else {
                    continue
                }
                node.simdTransform = simd_mul(restLocal, localDelta)
            }
        }
    }

    func applyAppearance(skinVariantId: String, enabledAccessoryIds: [String], textureOverrides: [String]) {
        activeSkinVariantId = skinVariantId
        applyAccessoryState(enabledAccessoryIds)
        applyTextureOverrides(textureOverrides)
    }

    private func normalizeModelTransform() {
        guard let modelNode else {
            return
        }
        let (minV, maxV) = modelNode.boundingBox
        let sx = maxV.x - minV.x
        let sy = maxV.y - minV.y
        let sz = maxV.z - minV.z
        let maxDim = max(0.001, max(sx, max(sy, sz)))
        if !sx.isFinite || !sy.isFinite || !sz.isFinite || maxDim < 0.001 {
            baseScale = 1.0
            modelNode.scale = SCNVector3(x: baseScale, y: baseScale, z: baseScale)
            modelNode.position = SCNVector3Zero
            modelBasePosition = modelNode.position
            modelNode.eulerAngles = SCNVector3(x: -0.08, y: modelFacingYaw, z: 0.0)
            tuneCameraForModel(scaledMaxDim: 1.0)
            return
        }
        baseScale = min(10.0, max(0.02, 0.72 / maxDim))
        modelNode.scale = SCNVector3(x: baseScale, y: baseScale, z: baseScale)
        let cx = (minV.x + maxV.x) * 0.5
        let cy = (minV.y + maxV.y) * 0.5
        let cz = (minV.z + maxV.z) * 0.5
        modelNode.position = SCNVector3(
            x: -(cx * baseScale),
            y: -(cy * baseScale),
            z: -(cz * baseScale))
        modelBasePosition = modelNode.position
        modelNode.eulerAngles = SCNVector3(x: -0.08, y: modelFacingYaw, z: 0.0)
        let scaledMaxDim = CGFloat(maxDim) * baseScale
        tuneCameraForModel(scaledMaxDim: scaledMaxDim)
    }

    private func tuneCameraForModel(scaledMaxDim: CGFloat) {
        let diameter = max(0.20, min(6.0, scaledMaxDim))
        let radius = diameter * 0.5
        let fovDeg = CGFloat(cameraNode.camera?.fieldOfView ?? 50.0)
        let halfFovRad = (fovDeg * .pi / 180.0) * 0.5
        let geometricDistance = radius / max(0.10, tan(halfFovRad))
        let distance = max(2.90, min(12.0, geometricDistance * 3.0))
        cameraNode.position = SCNVector3(x: 0.0, y: diameter * 0.10, z: distance)
    }

    private func loadScene(url: URL) -> SCNScene? {
        return try? SCNScene(url: url, options: nil)
    }

    private func containsRenderableGeometry(root: SCNNode) -> Bool {
        var hasRenderable = false
        root.enumerateChildNodes { node, stop in
            guard let geometry = node.geometry else {
                return
            }
            if !geometry.sources(for: .vertex).isEmpty {
                hasRenderable = true
                stop.pointee = true
            }
        }
        return hasRenderable
    }

    private func runActionPulse(node: SCNNode, actionCode: Int32, intensity: Float) {
        node.removeAction(forKey: "mfx_action_pulse")
        let amp = CGFloat(max(0.02, min(0.12, intensity * 0.16)))
        let up = SCNAction.scale(to: baseScale * (1.0 + amp), duration: actionCode == 2 ? 0.10 : 0.14)
        let down = SCNAction.scale(to: baseScale, duration: 0.16)
        up.timingMode = .easeOut
        down.timingMode = .easeInEaseOut
        node.runAction(SCNAction.sequence([up, down]), forKey: "mfx_action_pulse")
    }

    private func rebuildBoneNodeIndex(root: SCNNode) {
        boneNodesByName.removeAll(keepingCapacity: true)
        restLocalTransformByNode.removeAll(keepingCapacity: true)
        root.enumerateChildNodes { node, _ in
            guard let key = Self.normalizedBoneName(node.name), !key.isEmpty else {
                return
            }
            self.boneNodesByName[key, default: []].append(node)
            self.restLocalTransformByNode[ObjectIdentifier(node)] = node.simdTransform
        }
    }

    private func rebuildAccessoryNodeIndex(root: SCNNode) {
        accessoryNodesByName.removeAll(keepingCapacity: true)
        root.enumerateChildNodes { node, _ in
            guard let key = Self.normalizedBoneName(node.name), !key.isEmpty else {
                return
            }
            self.accessoryNodesByName[key, default: []].append(node)
        }
    }

    private func rebuildMaterialIndex(root: SCNNode) {
        materialEntriesByName.removeAll(keepingCapacity: true)
        originalDiffuseByMaterial.removeAll(keepingCapacity: true)
        root.enumerateChildNodes { node, _ in
            guard let geometry = node.geometry else {
                return
            }
            for (index, material) in geometry.materials.enumerated() {
                let fallbackName = (node.name ?? "node") + "#\(index)"
                let key = Self.normalizedBoneName(material.name ?? fallbackName)
                guard let key, !key.isEmpty else {
                    continue
                }
                material.isDoubleSided = true
                self.materialEntriesByName[key, default: []].append(material)
                let materialId = ObjectIdentifier(material)
                if self.originalDiffuseByMaterial[materialId] == nil {
                    self.originalDiffuseByMaterial[materialId] = material.diffuse.contents ?? NSColor.white
                }
            }
        }
    }

    private func applyAccessoryState(_ enabledAccessoryIds: [String]) {
        let normalizedEnabled = Set(enabledAccessoryIds.compactMap { Self.normalizedBoneName($0) })
        for key in normalizedEnabled {
            knownAccessoryNames.insert(key)
        }
        if knownAccessoryNames.isEmpty {
            return
        }

        for key in knownAccessoryNames {
            guard let nodes = accessoryNodesByName[key] else {
                continue
            }
            let shouldShow = normalizedEnabled.contains(key)
            for node in nodes {
                node.isHidden = !shouldShow
            }
        }
    }

    private func applyTextureOverrides(_ textureOverrides: [String]) {
        if materialEntriesByName.isEmpty {
            return
        }

        var overrideMap: [String: String] = [:]
        for raw in textureOverrides {
            guard let (materialKey, texturePath) = parseTextureOverride(raw) else {
                continue
            }
            overrideMap[materialKey] = texturePath
        }

        for (materialKey, materials) in materialEntriesByName {
            let overridePath = overrideMap[materialKey] ?? ""
            for material in materials {
                let materialId = ObjectIdentifier(material)
                if !overridePath.isEmpty, let image = NSImage(contentsOfFile: overridePath) {
                    material.diffuse.contents = image
                    continue
                }
                if let original = originalDiffuseByMaterial[materialId] {
                    material.diffuse.contents = original
                }
            }
        }
    }

    private func parseTextureOverride(_ raw: String) -> (String, String)? {
        let trimmed = raw.trimmingCharacters(in: .whitespacesAndNewlines)
        if trimmed.isEmpty {
            return nil
        }
        if let eqIndex = trimmed.firstIndex(of: "=") {
            let key = String(trimmed[..<eqIndex]).trimmingCharacters(in: .whitespacesAndNewlines)
            let value = String(trimmed[trimmed.index(after: eqIndex)...]).trimmingCharacters(in: .whitespacesAndNewlines)
            guard let normalizedKey = Self.normalizedBoneName(key), !normalizedKey.isEmpty else {
                return nil
            }
            return (normalizedKey, value)
        }
        if let colonIndex = trimmed.firstIndex(of: ":") {
            let key = String(trimmed[..<colonIndex]).trimmingCharacters(in: .whitespacesAndNewlines)
            let value = String(trimmed[trimmed.index(after: colonIndex)...]).trimmingCharacters(in: .whitespacesAndNewlines)
            guard let normalizedKey = Self.normalizedBoneName(key), !normalizedKey.isEmpty else {
                return nil
            }
            return (normalizedKey, value)
        }
        return nil
    }

    private static func normalizedBoneName(_ value: String?) -> String? {
        guard let value else {
            return nil
        }
        let trimmed = value.trimmingCharacters(in: .whitespacesAndNewlines)
        if trimmed.isEmpty {
            return nil
        }
        return trimmed.lowercased()
    }

    private func composeLocalDelta(
        positionX: Float,
        positionY: Float,
        positionZ: Float,
        rotationX: Float,
        rotationY: Float,
        rotationZ: Float,
        rotationW: Float,
        scaleX: Float,
        scaleY: Float,
        scaleZ: Float
    ) -> simd_float4x4 {
        var rotation = simd_quatf(
            ix: rotationX,
            iy: rotationY,
            iz: rotationZ,
            r: rotationW)
        let rotationVector = rotation.vector
        if !rotationVector.x.isFinite ||
            !rotationVector.y.isFinite ||
            !rotationVector.z.isFinite ||
            !rotationVector.w.isFinite ||
            simd_length_squared(rotationVector) < 0.000001 {
            rotation = simd_quatf(angle: 0.0, axis: SIMD3<Float>(0.0, 1.0, 0.0))
        } else {
            rotation = simd_normalize(rotation)
        }

        var scale = SIMD3<Float>(
            scaleX.isFinite ? scaleX : 1.0,
            scaleY.isFinite ? scaleY : 1.0,
            scaleZ.isFinite ? scaleZ : 1.0)
        if scale.x == 0.0 { scale.x = 1.0 }
        if scale.y == 0.0 { scale.y = 1.0 }
        if scale.z == 0.0 { scale.z = 1.0 }

        let translation = SIMD3<Float>(
            positionX.isFinite ? positionX : 0.0,
            positionY.isFinite ? positionY : 0.0,
            positionZ.isFinite ? positionZ : 0.0)

        var scaleMatrix = matrix_identity_float4x4
        scaleMatrix.columns.0.x = scale.x
        scaleMatrix.columns.1.y = scale.y
        scaleMatrix.columns.2.z = scale.z

        var translationMatrix = matrix_identity_float4x4
        translationMatrix.columns.3 = SIMD4<Float>(translation.x, translation.y, translation.z, 1.0)

        return simd_mul(simd_mul(translationMatrix, simd_float4x4(rotation)), scaleMatrix)
    }
}

@_cdecl("mfx_macos_mouse_companion_create_v1")
public func mfx_macos_mouse_companion_create_v1(_ sizePx: Int32) -> UnsafeMutableRawPointer? {
    if Thread.isMainThread {
        let bits = MainActor.assumeIsolated {
            UInt(bitPattern: Unmanaged.passRetained(MfxMouseCompanionHost(sizePx: Int(sizePx))).toOpaque())
        }
        return UnsafeMutableRawPointer(bitPattern: bits)
    }

    var bits: UInt = 0
    DispatchQueue.main.sync {
        bits = MainActor.assumeIsolated {
            UInt(bitPattern: Unmanaged.passRetained(MfxMouseCompanionHost(sizePx: Int(sizePx))).toOpaque())
        }
    }
    return UnsafeMutableRawPointer(bitPattern: bits)
}

@_cdecl("mfx_macos_mouse_companion_release_v1")
public func mfx_macos_mouse_companion_release_v1(_ handle: UnsafeMutableRawPointer?) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            let host = Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeRetainedValue()
            host.hide()
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            let host = Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeRetainedValue()
            host.hide()
        }
    }
}

@_cdecl("mfx_macos_mouse_companion_show_v1")
public func mfx_macos_mouse_companion_show_v1(_ handle: UnsafeMutableRawPointer?) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().show()
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().show()
        }
    }
}

@_cdecl("mfx_macos_mouse_companion_hide_v1")
public func mfx_macos_mouse_companion_hide_v1(_ handle: UnsafeMutableRawPointer?) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().hide()
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().hide()
        }
    }
}

@_cdecl("mfx_macos_mouse_companion_load_model_v1")
public func mfx_macos_mouse_companion_load_model_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ modelPathUtf8: UnsafePointer<CChar>?
) -> Int32 {
    guard let handle, let modelPathUtf8, let path = String(validatingCString: modelPathUtf8), !path.isEmpty else {
        return 0
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        let ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            return Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().loadModel(path: path)
        }
        return ok ? 1 : 0
    }

    var ok = false
    DispatchQueue.main.sync {
        ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            return Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().loadModel(path: path)
        }
    }
    return ok ? 1 : 0
}

@_cdecl("mfx_macos_mouse_companion_apply_appearance_v1")
public func mfx_macos_mouse_companion_apply_appearance_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ skinVariantIdUtf8: UnsafePointer<CChar>?,
    _ enabledAccessoryIdsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ accessoryCount: Int32,
    _ textureOverridePathsUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ textureOverrideCount: Int32
) -> Int32 {
    guard let handle else {
        return 0
    }
    let handleBits = UInt(bitPattern: handle)
    let skinVariantId = skinVariantIdUtf8.flatMap { String(validatingCString: $0) } ?? ""

    let safeAccessoryCount = max(0, Int(accessoryCount))
    var accessories: [String] = []
    accessories.reserveCapacity(safeAccessoryCount)
    if let enabledAccessoryIdsUtf8, safeAccessoryCount > 0 {
        for idx in 0..<safeAccessoryCount {
            let raw = enabledAccessoryIdsUtf8.advanced(by: idx).pointee
            if let raw, let value = String(validatingCString: raw), !value.isEmpty {
                accessories.append(value)
            }
        }
    }

    let safeTextureCount = max(0, Int(textureOverrideCount))
    var textures: [String] = []
    textures.reserveCapacity(safeTextureCount)
    if let textureOverridePathsUtf8, safeTextureCount > 0 {
        for idx in 0..<safeTextureCount {
            let raw = textureOverridePathsUtf8.advanced(by: idx).pointee
            if let raw, let value = String(validatingCString: raw), !value.isEmpty {
                textures.append(value)
            }
        }
    }

    if Thread.isMainThread {
        let ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().applyAppearance(
                skinVariantId: skinVariantId,
                enabledAccessoryIds: accessories,
                textureOverrides: textures)
            return true
        }
        return ok ? 1 : 0
    }

    var ok = false
    DispatchQueue.main.sync {
        ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().applyAppearance(
                skinVariantId: skinVariantId,
                enabledAccessoryIds: accessories,
                textureOverrides: textures)
            return true
        }
    }
    return ok ? 1 : 0
}

@_cdecl("mfx_macos_mouse_companion_configure_pose_binding_v1")
public func mfx_macos_mouse_companion_configure_pose_binding_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ boneNamesUtf8: UnsafePointer<UnsafePointer<CChar>?>?,
    _ boneCount: Int32
) -> Int32 {
    guard let handle else {
        return 0
    }
    let handleBits = UInt(bitPattern: handle)
    let safeCount = max(0, Int(boneCount))
    guard safeCount > 0, let boneNamesUtf8 else {
        return 0
    }

    var names: [String] = []
    names.reserveCapacity(safeCount)
    for idx in 0..<safeCount {
        let raw = boneNamesUtf8.advanced(by: idx).pointee
        if let raw {
            names.append(String(cString: raw))
        } else {
            names.append("")
        }
    }

    if Thread.isMainThread {
        let ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            return Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().configurePoseBinding(names: names)
        }
        return ok ? 1 : 0
    }

    var ok = false
    DispatchQueue.main.sync {
        ok = MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return false
            }
            return Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().configurePoseBinding(names: names)
        }
    }
    return ok ? 1 : 0
}

@_cdecl("mfx_macos_mouse_companion_configure_follow_profile_v1")
public func mfx_macos_mouse_companion_configure_follow_profile_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ offsetX: Int32,
    _ offsetY: Int32,
    _ pressLiftPx: Int32,
    _ edgeClampModeCode: Int32
) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().configureFollowProfile(
                offsetX: offsetX,
                offsetY: offsetY,
                pressLiftPx: pressLiftPx,
                edgeClampModeCode: edgeClampModeCode)
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().configureFollowProfile(
                offsetX: offsetX,
                offsetY: offsetY,
                pressLiftPx: pressLiftPx,
                edgeClampModeCode: edgeClampModeCode)
        }
    }
}

@_cdecl("mfx_macos_mouse_companion_update_state_v1")
public func mfx_macos_mouse_companion_update_state_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ cursorX: Int32,
    _ cursorY: Int32,
    _ actionCode: Int32,
    _ actionIntensity: Float,
    _ skeletonBoneCount: Int32
) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().update(
                cursorX: cursorX,
                cursorY: cursorY,
                actionCode: actionCode,
                actionIntensity: actionIntensity,
                skeletonBoneCount: skeletonBoneCount)
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().update(
                cursorX: cursorX,
                cursorY: cursorY,
                actionCode: actionCode,
                actionIntensity: actionIntensity,
                skeletonBoneCount: skeletonBoneCount)
        }
    }
}

@_cdecl("mfx_macos_mouse_companion_apply_pose_v1")
public func mfx_macos_mouse_companion_apply_pose_v1(
    _ handle: UnsafeMutableRawPointer?,
    _ boneIndices: UnsafePointer<Int32>?,
    _ positionsXYZ: UnsafePointer<Float>?,
    _ rotationsXYZW: UnsafePointer<Float>?,
    _ scalesXYZ: UnsafePointer<Float>?,
    _ poseCount: Int32
) {
    guard let handle else {
        return
    }
    let handleBits = UInt(bitPattern: handle)
    let safeCount = max(0, Int(poseCount))
    guard safeCount > 0,
        let boneIndices,
        let positionsXYZ,
        let rotationsXYZW,
        let scalesXYZ
    else {
        return
    }

    let copiedIndices = Array(UnsafeBufferPointer(start: boneIndices, count: safeCount))
    let copiedPositions = Array(UnsafeBufferPointer(start: positionsXYZ, count: safeCount * 3))
    let copiedRotations = Array(UnsafeBufferPointer(start: rotationsXYZW, count: safeCount * 4))
    let copiedScales = Array(UnsafeBufferPointer(start: scalesXYZ, count: safeCount * 3))

    if Thread.isMainThread {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().applyPose(
                indices: copiedIndices,
                positions: copiedPositions,
                rotations: copiedRotations,
                scales: copiedScales)
        }
        return
    }

    DispatchQueue.main.sync {
        MainActor.assumeIsolated {
            guard let raw = UnsafeMutableRawPointer(bitPattern: handleBits) else {
                return
            }
            Unmanaged<MfxMouseCompanionHost>.fromOpaque(raw).takeUnretainedValue().applyPose(
                indices: copiedIndices,
                positions: copiedPositions,
                rotations: copiedRotations,
                scales: copiedScales)
        }
    }
}
