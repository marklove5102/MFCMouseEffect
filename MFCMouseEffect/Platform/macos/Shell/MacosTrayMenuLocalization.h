#pragma once

#include <string>

namespace mousefx {

struct MacosTrayMenuText {
    std::string settingsTitle = "Settings";
    std::string exitTitle = "Exit";
    std::string tooltip = "MFCMouseEffect";
};

MacosTrayMenuText ResolveMacosTrayMenuText();

} // namespace mousefx
