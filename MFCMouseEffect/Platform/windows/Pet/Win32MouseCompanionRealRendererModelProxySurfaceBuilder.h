#pragma once

namespace mousefx::windows {

struct Win32MouseCompanionRealRendererAssetNodeWorldSpaceProfile;
struct Win32MouseCompanionRealRendererScene;

void BuildWin32MouseCompanionRealRendererModelProxySurfaces(
    const Win32MouseCompanionRealRendererAssetNodeWorldSpaceProfile& worldSpaceProfile,
    Win32MouseCompanionRealRendererScene& scene);

} // namespace mousefx::windows
