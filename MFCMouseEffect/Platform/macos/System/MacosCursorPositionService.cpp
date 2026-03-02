#include "pch.h"

#include "Platform/macos/System/MacosCursorPositionService.h"

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace mousefx {

bool MacosCursorPositionService::TryGetCursorScreenPoint(ScreenPoint* outPt) const {
    if (!outPt) {
        return false;
    }
#if defined(__APPLE__)
    CGEventRef event = CGEventCreate(nullptr);
    if (!event) {
        return false;
    }
    const CGPoint pos = CGEventGetLocation(event);
    CFRelease(event);
    outPt->x = static_cast<int32_t>(pos.x);
    outPt->y = static_cast<int32_t>(pos.y);
    return true;
#else
    return false;
#endif
}

} // namespace mousefx
