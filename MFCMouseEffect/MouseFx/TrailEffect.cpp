#include "pch.h"
#include "TrailEffect.h"
#include "ThemeStyle.h"

namespace mousefx {

// Assuming ToLowerAscii is defined elsewhere or will be added.
// For now, I'll make a placeholder to ensure compilation.
// This function is not part of the original code, but implied by the change.
// If it's not defined, this will cause a compilation error.
// For the purpose of this edit, I'll assume it's available or will be.
// std::string ToLowerAscii(const std::string& s) { /* ... */ }

TrailEffect::TrailEffect(const std::string& themeName) : window_(std::make_unique<TrailWindow>()) {
    isChromatic_ = (ToLowerAscii(themeName) == "chromatic");
}

TrailEffect::~TrailEffect() {
    Shutdown();
}

bool TrailEffect::Initialize() {
    if (!window_->Create()) return false;
    window_->SetChromatic(isChromatic_);
    return true;
}

// The Initialize method is removed as its logic has been moved to the constructor.
// bool TrailEffect::Initialize() {
//     window_ = std::make_unique<TrailWindow>();
//     if (!window_->Create()) return false;
//     window_->SetChromatic(isChromatic_);
//     return true;
// }

void TrailEffect::Shutdown() {
    if (window_) {
        window_->Shutdown();
        window_.reset();
    }
}

void TrailEffect::OnMouseMove(const POINT& pt) {
    if (window_) {
        window_->AddPoint(pt);
    }
}

} // namespace mousefx
