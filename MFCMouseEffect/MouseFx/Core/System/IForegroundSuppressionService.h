#pragma once

#include <cstdint>

namespace mousefx {

// Cross-platform foreground suppression strategy (e.g. VM / remote desktop).
class IForegroundSuppressionService {
public:
    virtual ~IForegroundSuppressionService() = default;

    virtual bool ShouldSuppress(uint64_t nowTickMs) = 0;
    virtual uint64_t CheckIntervalMsForDiagnostics() const { return 0; }
};

} // namespace mousefx
