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

    // FFDec raster bounds for symbol1825 (playerUI.SmallMapUIMC).
    static constexpr float kReferenceOriginX = 981.0F;
    static constexpr float kReferenceOriginY = 8.5F;

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
