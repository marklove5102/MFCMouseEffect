#pragma once

#include <cstdint>
#include <string>

namespace mousefx::wasm {

struct CommandExecutionResult final {
    uint32_t parsedCommands = 0;
    uint32_t executedTextCommands = 0;
    uint32_t executedImageCommands = 0;
    uint32_t executedPulseCommands = 0;
    uint32_t executedPolylineCommands = 0;
    uint32_t executedPathStrokeCommands = 0;
    uint32_t executedPathFillCommands = 0;
    uint32_t executedGlowBatchCommands = 0;
    uint32_t executedSpriteBatchCommands = 0;
    uint32_t executedGlowEmitterCommands = 0;
    uint32_t executedGlowEmitterRemoveCommands = 0;
    uint32_t executedSpriteEmitterCommands = 0;
    uint32_t executedSpriteEmitterRemoveCommands = 0;
    uint32_t executedParticleEmitterCommands = 0;
    uint32_t executedParticleEmitterRemoveCommands = 0;
    uint32_t executedRibbonTrailCommands = 0;
    uint32_t executedRibbonTrailRemoveCommands = 0;
    uint32_t executedQuadFieldCommands = 0;
    uint32_t executedQuadFieldRemoveCommands = 0;
    uint32_t executedGroupRemoveCommands = 0;
    uint32_t executedGroupPresentationCommands = 0;
    uint32_t executedGroupClipRectCommands = 0;
    uint32_t executedGroupLayerCommands = 0;
    uint32_t executedGroupTransformCommands = 0;
    uint32_t executedGroupLocalOriginCommands = 0;
    uint32_t executedGroupMaterialCommands = 0;
    uint32_t executedGroupPassCommands = 0;
    uint32_t throttledCommands = 0;
    uint32_t throttledByCapacityCommands = 0;
    uint32_t throttledByIntervalCommands = 0;
    uint32_t droppedCommands = 0;
    bool renderedAny = false;
    std::string lastError{};
};

} // namespace mousefx::wasm
