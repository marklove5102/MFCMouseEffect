#pragma once

#include "MouseFx/Core/Shell/IDpiAwarenessService.h"

namespace mousefx {

class MacosDpiAwarenessService final : public IDpiAwarenessService {
public:
    void EnableForScreenCoords() override;
};

} // namespace mousefx
