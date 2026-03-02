#include "Platform/macos/Shell/MacosTrayService.h"

namespace mousefx {

struct MacosTrayService::Impl {};

MacosTrayService::MacosTrayService()
    : impl_(std::make_unique<Impl>()) {}

MacosTrayService::~MacosTrayService() = default;

bool MacosTrayService::Start(IAppShellHost* host, bool showTrayIcon) {
    (void)showTrayIcon;
    return host != nullptr;
}

void MacosTrayService::Stop() {
}

void MacosTrayService::RequestExit() {
}

} // namespace mousefx
