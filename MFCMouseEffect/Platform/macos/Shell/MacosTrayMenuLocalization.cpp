#include "Platform/macos/Shell/MacosTrayMenuLocalization.h"

#if defined(__APPLE__)
#include "Platform/macos/Shell/MacosTrayMenuLocalizationSwiftBridge.h"
#endif

namespace mousefx {

MacosTrayMenuText BuildMacosTrayMenuText(bool preferZhLabels) {
    if (preferZhLabels) {
        MacosTrayMenuText text;
        text.starProjectTitle = u8"\u2605 Star \u9879\u76ee";
        text.settingsTitle = u8"\u8bbe\u7f6e...";
        text.exitTitle = u8"\u9000\u51fa";
        text.preferZhLabels = true;
        return text;
    }
    return {};
}

MacosTrayMenuText ResolveMacosTrayMenuText() {
#if defined(__APPLE__)
    if (mfx_macos_tray_prefers_zh_language_v1() > 0) {
        return BuildMacosTrayMenuText(true);
    }
#endif
    return BuildMacosTrayMenuText(false);
}

} // namespace mousefx
