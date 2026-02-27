import AppKit
import Foundation

@_cdecl("mfx_macos_open_settings_url")
public func mfx_macos_open_settings_url(_ urlUtf8: UnsafePointer<CChar>?) -> Bool {
    guard let urlUtf8 else {
        return false
    }

    let raw = String(cString: urlUtf8)
    guard let url = URL(string: raw) else {
        return false
    }
    return NSWorkspace.shared.open(url)
}
