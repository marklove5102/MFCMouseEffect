#include "pch.h"
#include "WebSettingsServer.AutomationRoutes.h"

#include <string>

#include "MouseFx/Server/HttpServer.h"
#include "MouseFx/Server/WebSettingsServer.AutomationCatalogRoutes.h"
#include "MouseFx/Server/WebSettingsServer.AutomationShortcutCaptureRoutes.h"

namespace mousefx {

bool HandleWebSettingsAutomationApiRoute(
    const HttpRequest& req,
    const std::string& path,
    AppController* controller,
    HttpResponse& resp) {
    if (HandleWebSettingsAutomationShortcutCaptureApiRoute(req, path, controller, resp)) {
        return true;
    }

    if (HandleWebSettingsAutomationCatalogApiRoute(req, path, controller, resp)) {
        return true;
    }

    return false;
}

} // namespace mousefx
