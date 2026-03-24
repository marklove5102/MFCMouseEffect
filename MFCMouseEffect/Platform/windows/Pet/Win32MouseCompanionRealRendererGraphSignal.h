#pragma once

#include <string>

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry;

float ResolveWin32MouseCompanionRealRendererGraphEntrySignal(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry& entry);
float ResolveWin32MouseCompanionRealRendererGraphSemanticSignal(
    const Win32MouseCompanionRealRendererAssetNodeMatchGraphEntry& entry,
    const std::string& expectedSemantic);

} // namespace mousefx::windows
