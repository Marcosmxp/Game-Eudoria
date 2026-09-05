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

    // The complete FFDec raster for symbol1825 also contains totalIcon (1815),
    // whose first-frame feature panel extends almost 1,000 px to the left.
    // Runtime must not draw that entire reference as one sprite. These source
    // coordinates isolate the payload-defined minimap chrome based on:
    //   background character 1632 bounds = [-181, 0] x [0, 192]
    //   symbol1825 FFDec raster origin    = (-981, -8.5)
    static constexpr float kChromeSourceX = 800.0F;
    static constexpr float kChromeSourceY = 8.5F;
    static constexpr float kChromeWidth = 181.0F;
    static constexpr float kChromeHeight = 192.0F;
    static constexpr float kChromeLocalX = -181.0F;
    static constexpr float kChromeLocalY = 0.0F;

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
