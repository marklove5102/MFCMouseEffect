#pragma once

#include "MouseFx/Core/Config/EffectConfig.h"
#include "MouseFx/Core/Protocol/InputTypes.h"

#include <string>
#include <vector>

namespace mousefx {

struct GestureSimilarityMetrics final {
    size_t strokeCount = 0;
    size_t pointCount = 0;
    double pathLengthPx = 0.0;
    double startEndDistancePx = 0.0;
};

bool IsCustomGesturePatternMode(const std::string& mode);

std::vector<std::vector<AutomationKeyBinding::GesturePoint>> GestureTemplateStrokesFromPattern(
    const AutomationKeyBinding::GesturePattern& pattern);

GestureSimilarityMetrics MeasureCapturedGesture(
    const std::vector<std::vector<ScreenPoint>>& capturedStrokes);

double ScoreGestureTemplateSimilarity(
    const std::vector<std::vector<AutomationKeyBinding::GesturePoint>>& templateStrokes,
    const std::vector<std::vector<ScreenPoint>>& capturedStrokes);

double ScorePresetGestureSimilarity(
    const std::string& normalizedActionId,
    const std::vector<ScreenPoint>& capturedStroke);

} // namespace mousefx
