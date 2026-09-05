#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

class SmallMapHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui",
        const std::filesystem::path& minimapRoot = "legacy_assets/runtime/minimap");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    [[nodiscard]] bool hasMinimap() const noexcept { return minimap_.valid(); }

private:
    bool loadMinimap(SpriteRenderer& renderer, const std::filesystem::path& minimapRoot);

    static constexpr eudoria::ui::Point kRoot{1200.0F, 0.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopRight;

    // Complete symbol1825 FFDec raster origin, recovered from the payload.
    static constexpr float kReferenceOriginX = 981.0F;
    static constexpr float kReferenceOriginY = 8.5F;

    // Right-side SmallMap chrome and direct controls. This region begins after
    // totalIcon's raster ends, so restoring it does not reintroduce the giant
    // feature-panel spill that the previous full-composite render caused.
    // source X 758 => local X -223 because symbol1825 raster origin is -981.
    static constexpr float kChromeSourceX = 758.0F;
    static constexpr float kChromeSourceY = 0.0F;
    static constexpr float kChromeWidth = 240.0F;
    static constexpr float kChromeHeight = 230.0F;
    static constexpr float kChromeLocalX = -223.0F;
    static constexpr float kChromeLocalY = -8.5F;

    // SmallMap.totalIcon is character 1815 placed at (-229.7, 47.95). Matching
    // the independent character1815 raster back into symbol1825 gives this
    // exact non-overlapping source region in the payload reference.
    static constexpr float kFeatureSourceX = 5.0F;
    static constexpr float kFeatureSourceY = 57.0F;
    static constexpr float kFeatureWidth = 752.0F;
    static constexpr float kFeatureHeight = 209.0F;
    static constexpr float kFeatureLocalX = -976.0F;
    static constexpr float kFeatureLocalY = 48.5F;

    // Exact values recovered from SmallMapUI.as and symbol1825:
    // mapRootPoint.x = -141, mapRootPoint.y = 34
    // mapRootPoint.scrollRect = Rectangle(0, 0, 125, 130)
    static constexpr float kViewportX = -141.0F;
    static constexpr float kViewportY = 34.0F;
    static constexpr float kViewportWidth = 125.0F;
    static constexpr float kViewportHeight = 130.0F;

    SpriteTexture skin_;
    SpriteTexture minimap_;
};

} // namespace eudoria::game::ui
