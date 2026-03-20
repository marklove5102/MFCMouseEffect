#pragma once

#include <string>

namespace mousefx {

struct MacosTrayMenuText {
    std::string starProjectTitle = u8"\u2605 Star Project";
    std::string settingsTitle = "Settings...";
    std::string exitTitle = "Exit";
    std::string tooltip = "MFCMouseEffect";
    bool preferZhLabels = false;
};

MacosTrayMenuText BuildMacosTrayMenuText(bool preferZhLabels);
MacosTrayMenuText ResolveMacosTrayMenuText();

} // namespace mousefx
