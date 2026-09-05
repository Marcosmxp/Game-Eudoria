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
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/small_map",
        const std::filesystem::path& minimapRoot = "legacy_assets/runtime/minimap");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    [[nodiscard]] bool hasMinimap() const noexcept { return minimap_.valid(); }

private:
    bool loadMinimap(SpriteRenderer& renderer, const std::filesystem::path& minimapRoot);

    static void drawPlaced(
        SpriteRenderer& renderer,
        const SpriteTexture& texture,
        const eudoria::ui::Point& root,
        const eudoria::ui::Point& placement,
        const eudoria::ui::Point& rasterOffset,
        float scale);

    static constexpr eudoria::ui::Point kRoot{1200.0F, 0.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopRight;

    // Exact values recovered from SmallMapUI.as and symbol1825.
    static constexpr float kViewportX = -141.0F;
    static constexpr float kViewportY = 34.0F;
    static constexpr float kViewportWidth = 125.0F;
    static constexpr float kViewportHeight = 130.0F;

    SpriteTexture base_;
    SpriteTexture minimap_;
    SpriteTexture playerCenter_;
    SpriteTexture zoomOut_;
    SpriteTexture zoomIn_;
    SpriteTexture onlineBonus_;
    SpriteTexture mapButton_;
    SpriteTexture remoteDisplay_;
    SpriteTexture worldMapButton_;
    SpriteTexture shop_;
    SpriteTexture daysPrompt_;
    SpriteTexture ranking_;
    SpriteTexture dayBonus_;
    SpriteTexture skillEffect_;
    SpriteTexture drgLottery_;
    SpriteTexture misc1694_;
    SpriteTexture result_;
    SpriteTexture totalIcon_;
    SpriteTexture collapse_;
};

} // namespace eudoria::game::ui
