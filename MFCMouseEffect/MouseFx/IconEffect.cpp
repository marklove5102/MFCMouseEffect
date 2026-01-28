#include "pch.h"
#include "IconEffect.h"
#include "ThemeStyle.h"
#include "StandardRenderers.h"

namespace mousefx {

IconEffect::IconEffect(const std::string& themeName) {
    style_ = GetThemePalette(themeName).icon;
}

IconEffect::~IconEffect() {
    Shutdown();
}

bool IconEffect::Initialize() {
    if (!pool_.Initialize(8)) {
        return false;
    }
    return true;
}

void IconEffect::Shutdown() {
    pool_.Shutdown();
}

void IconEffect::OnClick(const ClickEvent& event) {
    RenderParams params;
    params.loop = false;
    params.intensity = 1.0f;
    pool_.ShowRipple(event, style_, std::make_unique<StarRenderer>(), params);
}

} // namespace mousefx
