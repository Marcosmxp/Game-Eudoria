#include "game/ui/PlayerInfoHud.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace eudoria::game::ui {

bool PlayerInfoHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot,
    const std::filesystem::path& runtimeRoot) {
    referenceSkin_ = {};
    renderer.loadTexture((referenceRoot / L"player_info.reference.png").wstring(), referenceSkin_);

    const bool hpLoaded = loadFrames(renderer, runtimeRoot / L"hp", hpFrames_);
    const bool mpLoaded = loadFrames(renderer, runtimeRoot / L"mp", mpFrames_);
    return referenceSkin_.valid() || hpLoaded || mpLoaded;
}

void PlayerInfoHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    if (referenceSkin_.valid()) {
        renderer.draw(
            referenceSkin_,
            root.x - (kReferenceOriginX * scale),
            root.y - (kReferenceOriginY * scale),
            static_cast<float>(referenceSkin_.width) * scale,
            static_cast<float>(referenceSkin_.height) * scale);
    }

    drawBar(renderer, hpFrames_, frameFor(vitals_.hp, vitals_.hpMax), kHpBar, root, scale);
    drawBar(renderer, mpFrames_, frameFor(vitals_.mp, vitals_.mpMax), kMpBar, root, scale);
    drawBar(renderer, hpFrames_, frameFor(vitals_.petHp, vitals_.petHpMax), kPetHpBar, root, scale);
    drawBar(renderer, mpFrames_, frameFor(vitals_.petMp, vitals_.petMpMax), kPetMpBar, root, scale);
}

int PlayerInfoHud::frameFor(const std::int32_t value, const std::int32_t maximum) noexcept {
    if (maximum <= 0) {
        return 2;
    }

    const double ratio = std::clamp(static_cast<double>(value) / static_cast<double>(maximum), 0.0, 1.0);
    // PlayerInfoUI.hpChange/mpChange clamps the legacy MovieClip to frame 2.
    return std::clamp(static_cast<int>(ratio * 100.0), 2, 100);
}

void PlayerInfoHud::drawBar(
    SpriteRenderer& renderer,
    const std::array<SpriteTexture, kFrameCount>& frames,
    const int frame,
    const BarPlacement& placement,
    const eudoria::ui::Point& root,
    const float legacyScale) {
    const auto index = static_cast<std::size_t>(std::clamp(frame, 1, 100) - 1);
    const auto& texture = frames[index];
    if (!texture.valid()) {
        return;
    }

    const float localX = placement.position.x + placement.boundsMinX * placement.scaleX;
    const float localY = placement.position.y + placement.boundsMinY * placement.scaleY;

    renderer.draw(
        texture,
        root.x + localX * legacyScale,
        root.y + localY * legacyScale,
        static_cast<float>(texture.width) * std::abs(placement.scaleX) * legacyScale,
        static_cast<float>(texture.height) * std::abs(placement.scaleY) * legacyScale);
}

bool PlayerInfoHud::loadFrames(
    SpriteRenderer& renderer,
    const std::filesystem::path& root,
    std::array<SpriteTexture, kFrameCount>& frames) {
    bool loadedAny = false;
    for (std::size_t index = 0; index < frames.size(); ++index) {
        const auto file = root / (std::to_wstring(index + 1) + L".png");
        if (std::filesystem::exists(file) && renderer.loadTexture(file.wstring(), frames[index])) {
            loadedAny = true;
        }
    }
    return loadedAny;
}

} // namespace eudoria::game::ui
