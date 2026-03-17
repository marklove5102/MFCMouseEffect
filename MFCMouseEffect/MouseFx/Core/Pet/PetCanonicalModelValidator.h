#pragma once

#include <string>
#include <vector>

namespace mousefx::pet {

struct CanonicalModelValidationReport final {
    bool ok{false};
    int nodeCount{0};
    int skinCount{0};
    int materialCount{0};
    int meshCount{0};
    int jointCount{0};
    std::vector<std::string> warnings{};
    std::string error{};
};

bool ValidateCanonicalGlb(const std::string& glbPath, CanonicalModelValidationReport* outReport);

} // namespace mousefx::pet

