#pragma once

#include <memory>

namespace mousefx::windows {

class IWin32MouseCompanionRendererBackend;

std::unique_ptr<IWin32MouseCompanionRendererBackend> CreateDefaultWin32MouseCompanionRendererBackend();

} // namespace mousefx::windows
