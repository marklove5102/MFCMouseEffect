// CommandHandler.ApplySettings.cpp - apply_settings entry orchestration.

#include "pch.h"
#include "CommandHandler.h"

#include "AppController.h"
#include "CommandHandler.ApplySettings.Internal.h"

namespace mousefx {

void CommandHandler::HandleApplySettings(const std::string& jsonCmd) {
    command_handler_apply_settings::json payload;
    if (!command_handler_apply_settings::TryParsePayloadObject(jsonCmd, &payload)) {
        return;
    }

    command_handler_apply_settings::ApplyActiveSettings(payload, controller_);

    if (payload.contains("ui_language") && payload["ui_language"].is_string()) {
        controller_->SetUiLanguage(payload["ui_language"].get<std::string>());
    }

    command_handler_apply_settings::ApplyTextSettings(payload, controller_);
    command_handler_apply_settings::ApplyInputIndicatorSettings(payload, controller_);
    command_handler_apply_settings::ApplyAutomationSettings(payload, controller_);
    command_handler_apply_settings::ApplyWasmSettings(payload, controller_);

    if (payload.contains("hold_follow_mode") && payload["hold_follow_mode"].is_string()) {
        controller_->SetHoldFollowMode(payload["hold_follow_mode"].get<std::string>());
    }
    if (payload.contains("hold_presenter_backend") && payload["hold_presenter_backend"].is_string()) {
        controller_->SetHoldPresenterBackend(payload["hold_presenter_backend"].get<std::string>());
    }

    command_handler_apply_settings::ApplyTrailTuningSettings(payload, controller_);

    // Theme last (recreates themed effects).
    if (payload.contains("theme") && payload["theme"].is_string()) {
        controller_->SetTheme(payload["theme"].get<std::string>());
    }
}

} // namespace mousefx
