#include "pch.h"

#include "OverlayHostService.h"

#include "Platform/PlatformTarget.h"

#if MFX_PLATFORM_WINDOWS
#include "MouseFx/Interfaces/IRippleRenderer.h"
#include "MouseFx/Interfaces/ITrailRenderer.h"
#include "MouseFx/Layers/ParticleTrailOverlayLayer.h"
#include "MouseFx/Layers/RippleOverlayLayer.h"
#include "MouseFx/Layers/TextOverlayLayer.h"
#include "MouseFx/Layers/TrailOverlayLayer.h"
#include "Platform/PlatformEffectFallbackFactory.h"
#include "Platform/PlatformOverlayServicesFactory.h"
#include "Settings/EmojiUtils.h"
#endif

namespace {

#if MFX_PLATFORM_WINDOWS
bool HasEmojiStarter(const std::wstring& text) {
    for (size_t i = 0; i < text.size();) {
        const uint32_t cp = settings::NextCodePointUtf16(text, &i);
        if (cp == 0) {
            break;
        }
        if (settings::IsEmojiCodePoint(cp)) {
            return true;
        }
    }
    return false;
}
#endif

} // namespace

namespace mousefx {

OverlayHostService& OverlayHostService::Instance() {
    static OverlayHostService instance;
    return instance;
}

OverlayHostService::~OverlayHostService() = default;

bool OverlayHostService::Initialize() {
#if MFX_PLATFORM_WINDOWS
    if (hostBackend_) return true;
    hostBackend_ = platform::CreateOverlayHostBackend();
    if (!hostBackend_) {
        return false;
    }
    if (!hostBackend_->Create()) {
        hostBackend_.reset();
        return false;
    }
    return true;
#else
    return false;
#endif
}

void OverlayHostService::Shutdown() {
#if MFX_PLATFORM_WINDOWS
    if (!hostBackend_) return;
    rippleLayer_ = nullptr;
    textLayer_ = nullptr;
    if (textFallback_) {
        textFallback_->Shutdown();
    }
    hostBackend_->Shutdown();
    hostBackend_.reset();
#endif
}

IOverlayLayer* OverlayHostService::AttachLayer(std::unique_ptr<IOverlayLayer> layer) {
#if MFX_PLATFORM_WINDOWS
    if (!layer) return nullptr;
    if (!Initialize()) return nullptr;
    return hostBackend_->AddLayer(std::move(layer));
#else
    (void)layer;
    return nullptr;
#endif
}

TrailOverlayLayer* OverlayHostService::AttachTrailLayer(std::unique_ptr<ITrailRenderer> renderer, int durationMs, int maxPoints, bool isChromatic) {
#if MFX_PLATFORM_WINDOWS
    if (!renderer) return nullptr;
    auto layer = std::make_unique<TrailOverlayLayer>(
        std::move(renderer),
        durationMs,
        maxPoints,
        Gdiplus::Color(220, 100, 255, 218),
        isChromatic);
    return static_cast<TrailOverlayLayer*>(AttachLayer(std::move(layer)));
#else
    (void)renderer;
    (void)durationMs;
    (void)maxPoints;
    (void)isChromatic;
    return nullptr;
#endif
}

ParticleTrailOverlayLayer* OverlayHostService::AttachParticleTrailLayer(bool isChromatic) {
#if MFX_PLATFORM_WINDOWS
    auto layer = std::make_unique<ParticleTrailOverlayLayer>(isChromatic);
    return static_cast<ParticleTrailOverlayLayer*>(AttachLayer(std::move(layer)));
#else
    (void)isChromatic;
    return nullptr;
#endif
}

uint64_t OverlayHostService::ShowRipple(const ClickEvent& ev, const RippleStyle& style, std::unique_ptr<IRippleRenderer> renderer, const RenderParams& params) {
#if MFX_PLATFORM_WINDOWS
    RippleOverlayLayer* layer = EnsureRippleLayer();
    if (!layer) return 0;
    return layer->ShowRipple(ev, style, std::move(renderer), params);
#else
    (void)ev;
    (void)style;
    (void)renderer;
    (void)params;
    return 0;
#endif
}

uint64_t OverlayHostService::ShowContinuousRipple(const ClickEvent& ev, const RippleStyle& style, std::unique_ptr<IRippleRenderer> renderer, const RenderParams& params) {
#if MFX_PLATFORM_WINDOWS
    RippleOverlayLayer* layer = EnsureRippleLayer();
    if (!layer) return 0;
    return layer->ShowContinuous(ev, style, std::move(renderer), params);
#else
    (void)ev;
    (void)style;
    (void)renderer;
    (void)params;
    return 0;
#endif
}

void OverlayHostService::UpdateRipplePosition(uint64_t id, const ScreenPoint& pt) {
#if MFX_PLATFORM_WINDOWS
    if (!rippleLayer_) return;
    rippleLayer_->UpdatePosition(id, pt);
#else
    (void)id;
    (void)pt;
#endif
}

void OverlayHostService::StopRipple(uint64_t id) {
#if MFX_PLATFORM_WINDOWS
    if (!rippleLayer_) return;
    rippleLayer_->Stop(id);
#else
    (void)id;
#endif
}

bool OverlayHostService::IsRippleActive(uint64_t id) const {
#if MFX_PLATFORM_WINDOWS
    if (!rippleLayer_) return false;
    return rippleLayer_->IsActive(id);
#else
    (void)id;
    return false;
#endif
}

void OverlayHostService::SendRippleCommand(uint64_t id, const std::string& cmd, const std::string& args) {
#if MFX_PLATFORM_WINDOWS
    if (!rippleLayer_) return;
    rippleLayer_->SendCommand(id, cmd, args);
#else
    (void)id;
    (void)cmd;
    (void)args;
#endif
}

void OverlayHostService::BroadcastRippleCommand(const std::string& cmd, const std::string& args) {
#if MFX_PLATFORM_WINDOWS
    if (!rippleLayer_) return;
    rippleLayer_->BroadcastCommand(cmd, args);
#else
    (void)cmd;
    (void)args;
#endif
}

bool OverlayHostService::ShowText(const ScreenPoint& pt, const std::wstring& text, Argb color, const TextConfig& config) {
#if MFX_PLATFORM_WINDOWS
    if (HasEmojiStarter(text)) {
        if (!textFallback_) {
            textFallback_ = platform::CreateTextEffectFallback();
        }
        if (!textFallback_ || !textFallback_->EnsureInitialized(8)) {
            return false;
        }
        textFallback_->ShowText(pt, text, color, config);
        return true;
    }
    TextOverlayLayer* layer = EnsureTextLayer();
    if (!layer) return false;
    layer->ShowText(pt, text, color, config);
    return true;
#else
    (void)pt;
    (void)text;
    (void)color;
    (void)config;
    return false;
#endif
}

void OverlayHostService::DetachLayer(IOverlayLayer* layer) {
#if MFX_PLATFORM_WINDOWS
    if (!hostBackend_ || !layer) return;
    if (rippleLayer_ == layer) {
        rippleLayer_ = nullptr;
    }
    if (textLayer_ == layer) {
        textLayer_ = nullptr;
    }
    hostBackend_->RemoveLayer(layer);
#else
    (void)layer;
#endif
}

RippleOverlayLayer* OverlayHostService::EnsureRippleLayer() {
#if MFX_PLATFORM_WINDOWS
    if (rippleLayer_) return rippleLayer_;
    auto layer = std::make_unique<RippleOverlayLayer>();
    rippleLayer_ = static_cast<RippleOverlayLayer*>(AttachLayer(std::move(layer)));
    return rippleLayer_;
#else
    return nullptr;
#endif
}

TextOverlayLayer* OverlayHostService::EnsureTextLayer() {
#if MFX_PLATFORM_WINDOWS
    if (textLayer_) return textLayer_;
    auto layer = std::make_unique<TextOverlayLayer>();
    textLayer_ = static_cast<TextOverlayLayer*>(AttachLayer(std::move(layer)));
    return textLayer_;
#else
    return nullptr;
#endif
}

} // namespace mousefx
