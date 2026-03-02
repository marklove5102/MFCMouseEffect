#pragma once

#include "MouseFx/Core/System/ICursorPositionService.h"

namespace mousefx {

class MacosCursorPositionService final : public ICursorPositionService {
public:
    bool TryGetCursorScreenPoint(ScreenPoint* outPt) const override;
};

} // namespace mousefx
