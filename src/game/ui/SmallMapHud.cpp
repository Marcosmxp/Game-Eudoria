#include "game/ui/SmallMapHud.h"

#include <algorithm>

namespace eudoria::game::ui {

bool SmallMapHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot,
    const std::filesystem::path& minimapRoot) {
    const bool skinLoaded = renderer.loadTexture(
        (referenceRoot / L"small_map.reference.png").wstring(), skin_);
    loadMinimap(renderer, minimapRoot);
    return skinLoaded;
}

bool SmallMapHud::loadMinimap(SpriteRenderer& renderer, const std::filesystem::path& minimapRoot) {
    minimap_ = {};
    if (!std::filesystem::exists(minimapRoot)) {
        return false;
    }

    const auto current = minimapRoot / L"current.jpg";
    if (std::filesystem::exists(current) && renderer.loadTexture(current.wstring(), minimap_)) {
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(minimapRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto extension = entry.path().extension().wstring();
        if (extension != L".jpg" && extension != L".jpeg" && extension != L".png") {
            continue;
        }
        if (renderer.loadTexture(entry.path().wstring(), minimap_)) {
            return true;
        }
    }

    return false;
}

void SmallMapHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    if (skin_.valid()) {
        renderer.draw(
            skin_,
            root.x - (kReferenceOriginX * scale),
            root.y - (kReferenceOriginY * scale),
            static_cast<float>(skin_.width) * scale,
            static_cast<float>(skin_.height) * scale);
    }

    if (minimap_.valid()) {
        renderer.draw(
            minimap_,
            root.x + (kViewportX * scale),
            root.y + (kViewportY * scale),
            kViewportWidth * scale,
            kViewportHeight * scale);
    }
}

} // namespace eudoria::game::ui
