#pragma once

#include "MouseFx/Core/Shell/ITrayService.h"

#include <memory>

namespace mousefx {

class MacosTrayService final : public ITrayService {
public:
    MacosTrayService();
    ~MacosTrayService() override;

    MacosTrayService(const MacosTrayService&) = delete;
    MacosTrayService& operator=(const MacosTrayService&) = delete;

    bool Start(IAppShellHost* host, bool showTrayIcon) override;
    void Stop() override;
    void RequestExit() override;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_{};
};

} // namespace mousefx
