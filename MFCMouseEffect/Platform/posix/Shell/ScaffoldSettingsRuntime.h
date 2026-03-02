#pragma once

#include <functional>
#include <memory>
#include <string>

namespace mousefx::platform {

class ScaffoldSettingsRuntime final {
public:
    using WarningSink = std::function<void(const std::string& title, const std::string& message)>;

    ScaffoldSettingsRuntime();
    ~ScaffoldSettingsRuntime();

    ScaffoldSettingsRuntime(const ScaffoldSettingsRuntime&) = delete;
    ScaffoldSettingsRuntime& operator=(const ScaffoldSettingsRuntime&) = delete;

    void SetRuntimeMode(bool trayAvailable, bool backgroundMode);
    bool Start(const WarningSink& warningSink);
    void Stop();

    const std::string& SettingsUrl() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mousefx::platform
