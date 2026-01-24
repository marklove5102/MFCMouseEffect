#include "pch.h"
#include "ParticleTrailEffect.h"

namespace mousefx {

ParticleTrailEffect::~ParticleTrailEffect() {
    Shutdown();
}

bool ParticleTrailEffect::Initialize() {
    window_ = std::make_unique<ParticleTrailWindow>();
    return window_->Create();
}

void ParticleTrailEffect::Shutdown() {
    if (window_) {
        window_->Shutdown();
        window_.reset();
    }
}

void ParticleTrailEffect::OnMouseMove(const POINT& pt) {
    if (window_) {
        window_->Emit(pt, 6); // Emit 6 particles per move event
    }
}

} // namespace mousefx
