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
        // Restore the direct SmallMap chrome/buttons as their own non-overlapping
        // payload region. This includes the minimap frame, vertical utility
        // buttons, map/world-map row, Claim/online-bonus area and collapse arrow,
        // but excludes character1815 (totalIcon).
        renderer.drawRegion(
            skin_,
            {
                kChromeSourceX,
                kChromeSourceY,
                kChromeWidth,
                kChromeHeight,
            },
            root.x + (kChromeLocalX * scale),
            root.y + (kChromeLocalY * scale),
            kChromeWidth * scale,
            kChromeHeight * scale);
    }

    // mapRootPoint.scrollRect in SmallMapUI.as is exactly 125 x 130 at
    // (-141, 34). The current pre-world-state minimap is still fitted into
    // that viewport; world-coordinate scrolling is connected later when the
    // local map state owns mapWidth/mapHeight/player rootPX/rootPY.
    if (minimap_.valid()) {
        renderer.draw(
            minimap_,
            root.x + (kViewportX * scale),
            root.y + (kViewportY * scale),
            kViewportWidth * scale,
            kViewportHeight * scale);
    }

    if (skin_.valid()) {
        // SmallMapUI starts with totalIcon expanded (mg = true). The legacy
        // sprite is intentionally restored independently instead of returning
        // to the old full-symbol composite. Its raster ends immediately before
        // the right-side chrome region, so the two components can be rendered
        // at their exact payload positions without duplicated pixels.
        renderer.drawRegion(
            skin_,
            {
                kFeatureSourceX,
                kFeatureSourceY,
                kFeatureWidth,
                kFeatureHeight,
            },
            root.x + (kFeatureLocalX * scale),
            root.y + (kFeatureLocalY * scale),
            kFeatureWidth * scale,
            kFeatureHeight * scale);
    }
}

} // namespace eudoria::game::ui
