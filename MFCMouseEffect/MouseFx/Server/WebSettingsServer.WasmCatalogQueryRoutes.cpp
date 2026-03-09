#include "pch.h"
#include "WebSettingsServer.WasmCatalogQueryRoutes.h"

#include <string>
#include <vector>

#include "MouseFx/Core/Control/AppController.h"
#include "MouseFx/Core/Wasm/WasmPluginCatalog.h"
#include "MouseFx/Core/Wasm/WasmPluginPaths.h"
#include "MouseFx/Server/HttpServer.h"
#include "MouseFx/Server/WebSettingsServer.WasmRouteUtils.h"
#include "MouseFx/Utils/StringUtils.h"

using json = nlohmann::json;

namespace mousefx {
using websettings_wasm_routes::SetJsonResponse;

namespace {

json InputKindsToJson(uint32_t mask) {
    json kinds = json::array();
    if (mask & wasm::kManifestInputKindClickBit) {
        kinds.push_back("click");
    }
    if (mask & wasm::kManifestInputKindMoveBit) {
        kinds.push_back("move");
    }
    if (mask & wasm::kManifestInputKindScrollBit) {
        kinds.push_back("scroll");
    }
    if (mask & wasm::kManifestInputKindHoldStartBit) {
        kinds.push_back("hold_start");
    }
    if (mask & wasm::kManifestInputKindHoldUpdateBit) {
        kinds.push_back("hold_update");
    }
    if (mask & wasm::kManifestInputKindHoldEndBit) {
        kinds.push_back("hold_end");
    }
    if (mask & wasm::kManifestInputKindHoverStartBit) {
        kinds.push_back("hover_start");
    }
    if (mask & wasm::kManifestInputKindHoverEndBit) {
        kinds.push_back("hover_end");
    }
    return kinds;
}

} // namespace

bool HandleWebSettingsWasmCatalogQueryApiRoute(
    const HttpRequest& req,
    const std::string& path,
    AppController* controller,
    HttpResponse& resp) {
    if (req.method != "POST" || path != "/api/wasm/catalog") {
        return false;
    }

    wasm::WasmPluginCatalog catalog;
    std::wstring configuredCatalogRoot;
    if (controller) {
        configuredCatalogRoot = Utf8ToWString(controller->GetConfigSnapshot().wasm.catalogRootPath);
    }
    const std::vector<std::wstring> searchRoots = wasm::WasmPluginPaths::ResolveSearchRoots(configuredCatalogRoot);
    const wasm::PluginCatalogResult result = catalog.DiscoverFromRoots(searchRoots);

    json plugins = json::array();
    for (const auto& plugin : result.plugins) {
        plugins.push_back({
            {"id", plugin.manifest.id},
            {"name", plugin.manifest.name},
            {"version", plugin.manifest.version},
            {"api_version", plugin.manifest.apiVersion},
            {"input_kinds", InputKindsToJson(plugin.manifest.inputKindsMask)},
            {"enable_frame_tick", plugin.manifest.enableFrameTick},
            {"manifest_path", Utf16ToUtf8(plugin.manifestPath.c_str())},
            {"wasm_path", Utf16ToUtf8(plugin.wasmPath.c_str())},
        });
    }

    json errors = json::array();
    for (const auto& error : result.errors) {
        errors.push_back(error);
    }

    json roots = json::array();
    for (const std::wstring& root : searchRoots) {
        roots.push_back(Utf16ToUtf8(root.c_str()));
    }

    SetJsonResponse(resp, json({
        {"ok", true},
        {"plugins", plugins},
        {"errors", errors},
        {"search_roots", roots},
        {"count", plugins.size()},
        {"error_count", errors.size()},
    }).dump());
    return true;
}

} // namespace mousefx
