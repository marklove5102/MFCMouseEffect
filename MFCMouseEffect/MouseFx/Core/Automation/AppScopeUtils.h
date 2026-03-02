#pragma once

#include "MouseFx/Utils/StringUtils.h"
#include "Platform/PlatformTarget.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace mousefx::automation_scope {

inline constexpr const char kProcessScopePrefix[] = "process:";
inline constexpr size_t kProcessScopePrefixLength = sizeof(kProcessScopePrefix) - 1;

inline bool IsGlobalScopeToken(const std::string& rawScope) {
    const std::string scope = ToLowerAscii(TrimAscii(rawScope));
    return scope.empty() || scope == "all" || scope == "*" || scope == "global";
}

inline bool IsProcessScopeToken(const std::string& rawScope) {
    const std::string scope = ToLowerAscii(TrimAscii(rawScope));
    return scope.rfind(kProcessScopePrefix, 0) == 0;
}

inline std::string NormalizeProcessName(std::string value) {
    value = ToLowerAscii(TrimAscii(std::move(value)));
    if (value.empty()) {
        return {};
    }

    const bool hasQuotePair =
        (value.size() >= 2) &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''));
    if (hasQuotePair) {
        value = value.substr(1, value.size() - 2);
    }
    if (value.empty()) {
        return {};
    }

    std::replace(value.begin(), value.end(), '\\', '/');
    const size_t slashPos = value.find_last_of('/');
    if (slashPos != std::string::npos) {
        value = value.substr(slashPos + 1);
    }

    value = ToLowerAscii(TrimAscii(std::move(value)));
    return value;
}

inline std::string ScopeProcessName(const std::string& rawScope) {
    const std::string scope = ToLowerAscii(TrimAscii(rawScope));
    if (scope.rfind(kProcessScopePrefix, 0) != 0) {
        return {};
    }
    return NormalizeProcessName(scope.substr(kProcessScopePrefixLength));
}

inline std::string NormalizeScopeToken(std::string value) {
    value = ToLowerAscii(TrimAscii(std::move(value)));
    if (IsGlobalScopeToken(value)) {
        return "all";
    }

    std::string processName;
    if (value.rfind(kProcessScopePrefix, 0) == 0) {
        processName = value.substr(kProcessScopePrefixLength);
    } else {
        processName = value;
    }

    processName = NormalizeProcessName(std::move(processName));
    if (processName.empty()) {
        return "all";
    }

#if MFX_PLATFORM_WINDOWS
    if (processName.find('.') == std::string::npos) {
        processName += ".exe";
    }
#endif
    return std::string(kProcessScopePrefix) + processName;
}

inline bool EndsWith(std::string_view value, std::string_view suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline void PushUniqueAlias(std::vector<std::string>* aliases, std::string value) {
    if (!aliases || value.empty()) {
        return;
    }
    if (std::find(aliases->begin(), aliases->end(), value) != aliases->end()) {
        return;
    }
    aliases->push_back(std::move(value));
}

inline std::vector<std::string> BuildProcessAliases(std::string value) {
    std::vector<std::string> aliases;
    const std::string normalized = NormalizeProcessName(std::move(value));
    if (normalized.empty()) {
        return aliases;
    }

    PushUniqueAlias(&aliases, normalized);

    const bool hasDot = normalized.find('.') != std::string::npos;
    const bool hasExeSuffix = EndsWith(normalized, ".exe");
    const bool hasAppSuffix = EndsWith(normalized, ".app");

    if (hasExeSuffix || hasAppSuffix) {
        const std::string base = normalized.substr(0, normalized.size() - 4);
        if (!base.empty()) {
            PushUniqueAlias(&aliases, base);
        }
    } else if (!hasDot) {
        PushUniqueAlias(&aliases, normalized + ".exe");
        PushUniqueAlias(&aliases, normalized + ".app");
    }

    return aliases;
}

inline bool IsSameProcessName(const std::string& lhs, const std::string& rhs) {
    const std::vector<std::string> lhsAliases = BuildProcessAliases(lhs);
    const std::vector<std::string> rhsAliases = BuildProcessAliases(rhs);
    if (lhsAliases.empty() || rhsAliases.empty()) {
        return false;
    }

    for (const std::string& alias : lhsAliases) {
        if (std::find(rhsAliases.begin(), rhsAliases.end(), alias) != rhsAliases.end()) {
            return true;
        }
    }
    return false;
}

inline bool AppScopeMatchesProcess(
    const std::vector<std::string>& appScopes,
    const std::string& processBaseName) {
    if (appScopes.empty()) {
        return true;
    }

    const std::string current = NormalizeProcessName(processBaseName);
    bool hasProcessScope = false;

    for (const std::string& rawScope : appScopes) {
        const std::string scope = NormalizeScopeToken(rawScope);
        if (IsGlobalScopeToken(scope)) {
            return true;
        }
        if (!IsProcessScopeToken(scope)) {
            return true;
        }

        const std::string expected = ScopeProcessName(scope);
        if (expected.empty()) {
            continue;
        }

        hasProcessScope = true;
        if (!current.empty() && IsSameProcessName(current, expected)) {
            return true;
        }
    }

    return !hasProcessScope;
}

inline int AppScopeSpecificity(const std::vector<std::string>& appScopes) {
    if (appScopes.empty()) {
        return 0;
    }

    bool hasProcessScope = false;
    for (const std::string& rawScope : appScopes) {
        const std::string scope = NormalizeScopeToken(rawScope);
        if (IsGlobalScopeToken(scope)) {
            return 0;
        }
        if (!IsProcessScopeToken(scope)) {
            return 0;
        }
        if (!ScopeProcessName(scope).empty()) {
            hasProcessScope = true;
        }
    }
    return hasProcessScope ? 1 : 0;
}

} // namespace mousefx::automation_scope
