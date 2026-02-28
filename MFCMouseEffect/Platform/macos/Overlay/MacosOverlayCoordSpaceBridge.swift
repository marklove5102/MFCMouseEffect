@preconcurrency import AppKit
@preconcurrency import ApplicationServices
@preconcurrency import Foundation

private let mfxNSScreenNumberKey = NSDeviceDescriptionKey("NSScreenNumber")

private func mfxClampToInt32(_ value: Double) -> Int32 {
    let rounded = value.rounded()
    if rounded < Double(Int32.min) {
        return Int32.min
    }
    if rounded > Double(Int32.max) {
        return Int32.max
    }
    return Int32(rounded)
}

@MainActor
private func mfxConvertQuartzToCocoaOnMainThread(_ inX: Int32, _ inY: Int32) -> (Bool, Int32, Int32) {
    let screens = NSScreen.screens
    if screens.isEmpty {
        return (false, inX, inY)
    }

    let quartzPoint = CGPoint(x: CGFloat(inX), y: CGFloat(inY))
    var matchedScreen: NSScreen?
    var matchedBounds = CGRect.zero

    for screen in screens {
        guard
            let screenNumber = screen.deviceDescription[mfxNSScreenNumberKey] as? NSNumber
        else {
            continue
        }
        let displayId = CGDirectDisplayID(screenNumber.uint32Value)
        let bounds = CGDisplayBounds(displayId)
        if bounds.contains(quartzPoint) {
            matchedScreen = screen
            matchedBounds = bounds
            break
        }
    }

    if matchedScreen == nil {
        matchedScreen = NSScreen.main ?? screens.first
        if
            let fallbackScreen = matchedScreen,
            let screenNumber = fallbackScreen.deviceDescription[mfxNSScreenNumberKey] as? NSNumber {
            matchedBounds = CGDisplayBounds(CGDirectDisplayID(screenNumber.uint32Value))
        }
    }

    guard let screen = matchedScreen else {
        return (false, inX, inY)
    }

    let frame = screen.frame
    if
        matchedBounds.width <= 0 ||
        matchedBounds.height <= 0 ||
        frame.width <= 0 ||
        frame.height <= 0 {
        return (true, inX, inY)
    }

    let scaleX = frame.width / matchedBounds.width
    let scaleY = frame.height / matchedBounds.height
    let localX = (CGFloat(inX) - matchedBounds.origin.x) * scaleX
    let localY = (CGFloat(inY) - matchedBounds.origin.y) * scaleY

    let cocoaX = frame.origin.x + localX
    let cocoaY = frame.origin.y + frame.height - localY
    return (true, mfxClampToInt32(cocoaX), mfxClampToInt32(cocoaY))
}

@_cdecl("mfx_macos_overlay_quartz_to_cocoa_v1")
public func mfx_macos_overlay_quartz_to_cocoa_v1(
    _ inX: Int32,
    _ inY: Int32,
    _ outX: UnsafeMutablePointer<Int32>?,
    _ outY: UnsafeMutablePointer<Int32>?
) -> Int32 {
    guard let outX, let outY else {
        return 0
    }

    let result: (Bool, Int32, Int32)
    if Thread.isMainThread {
        result = MainActor.assumeIsolated {
            mfxConvertQuartzToCocoaOnMainThread(inX, inY)
        }
    } else {
        var asyncResult: (Bool, Int32, Int32) = (false, inX, inY)
        DispatchQueue.main.sync {
            asyncResult = MainActor.assumeIsolated {
                mfxConvertQuartzToCocoaOnMainThread(inX, inY)
            }
        }
        result = asyncResult
    }

    if !result.0 {
        return 0
    }

    outX.pointee = result.1
    outY.pointee = result.2
    return 1
}

