#pragma once

#include "MouseFx/Server/HttpServer.h"
#include "Platform/posix/Shell/ScaffoldSettingsRequestHandler.h"
#include "Platform/posix/Shell/ScaffoldSettingsRuntime.h"

#include <memory>

namespace mousefx::platform::scaffold::runtime_internal {

bool StartEmbeddedServer(
    SettingsRequestHandler& requestHandler,
    std::unique_ptr<HttpServer>& server,
    const ScaffoldSettingsRuntime::WarningSink& warningSink);

} // namespace mousefx::platform::scaffold::runtime_internal
