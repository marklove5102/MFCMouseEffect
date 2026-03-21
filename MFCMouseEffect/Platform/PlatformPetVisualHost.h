#pragma once

#include <memory>

namespace mousefx {
class IPetVisualHost;
}

namespace mousefx::platform {

std::unique_ptr<IPetVisualHost> CreatePetVisualHost();

} // namespace mousefx::platform
