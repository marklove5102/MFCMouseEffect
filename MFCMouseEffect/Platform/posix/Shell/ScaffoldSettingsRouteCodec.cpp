#include "pch.h"

#include "Platform/posix/Shell/ScaffoldSettingsRouteCodec.h"

#include <string>

namespace mousefx::platform::scaffold {
namespace {

int HexDigitToInt(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

} // namespace

bool IsSettingsUrlShellSafe(std::string_view url) {
    if (url.empty()) {
        return false;
    }
    for (char c : url) {
        if (c == '"' || c == '`' || c == '$' || c == '\n' || c == '\r') {
            return false;
        }
    }
    return true;
}

bool TryParseUnsignedPort(std::string_view text, uint16_t* outPort) {
    if (!outPort || text.empty() || text.size() > 5) {
        return false;
    }

    uint32_t value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10u + static_cast<uint32_t>(c - '0');
        if (value > 65535u) {
            return false;
        }
    }

    if (value == 0u) {
        return false;
    }
    *outPort = static_cast<uint16_t>(value);
    return true;
}

std::string UrlDecodePercentCopy(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = HexDigitToInt(value[i + 1]);
            const int lo = HexDigitToInt(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

std::string UrlEncodeQueryValue(std::string_view value) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size() * 3);
    for (unsigned char c : value) {
        const bool isAlphaNum = (c >= 'a' && c <= 'z') ||
                                (c >= 'A' && c <= 'Z') ||
                                (c >= '0' && c <= '9');
        const bool isUnreserved = isAlphaNum || c == '-' || c == '_' || c == '.' || c == '~';
        if (isUnreserved) {
            out.push_back(static_cast<char>(c));
            continue;
        }
        out.push_back('%');
        out.push_back(kHex[(c >> 4) & 0x0f]);
        out.push_back(kHex[c & 0x0f]);
    }
    return out;
}

} // namespace mousefx::platform::scaffold
