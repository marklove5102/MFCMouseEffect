#include "pch.h"

#include "Platform/windows/Pet/Win32MouseCompanionRealRendererNodeMatchNaming.h"

#include <cctype>

namespace mousefx::windows {

std::string NormalizeWin32MouseCompanionNodeMatchToken(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return normalized;
}

std::vector<std::string> TokenizeWin32MouseCompanionNodeMatchText(const std::string& value) {
    std::vector<std::string> tokens;
    std::string current;
    current.reserve(value.size());

    auto flush = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            current.push_back(static_cast<char>(std::tolower(ch)));
            continue;
        }
        flush();
    }
    flush();
    return tokens;
}

} // namespace mousefx::windows
