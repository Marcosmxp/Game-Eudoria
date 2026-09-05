#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <chrono>
#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

class SmallMapHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/small_map",
        const std::filesystem::path& minimapRoot = "legacy_assets/runtime/minimap");

    void update() noexcept;
    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    bool onMouseUp(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    [[nodiscard]] bool hasMinimap() const noexcept { return minimap_.valid(); }
    [[nodiscard]] bool featurePanelExpanded() const noexcept { return featureExpanded_; }

private:
    struct Rect final {
        float left;
        float top;
        float right;
        float bottom;

        [[nodiscard]] bool contains(const eudoria::ui::Point point) const noexcept {
            return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
        }
    };

    bool loadMinimap(SpriteRenderer& renderer, const std::filesystem::path& minimapRoot);

    static void drawPlaced(
        SpriteRenderer& renderer,
        const SpriteTexture& texture,
        const eudoria::ui::Point& root,
        const eudoria::ui::Point& placement,
        const eudoria::ui::Point& rasterOffset,
        float scale);

    [[nodiscard]] static eudoria::ui::Point toLocal(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    void animateFeaturePanel(bool expand) noexcept;

    static constexpr eudoria::ui::Point kRoot{1200.0F, 0.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopRight;

    // Exact values recovered from SmallMapUI.as and symbol1825.
    static constexpr float kViewportX = -141.0F;
    static constexpr float kViewportY = 34.0F;
    static constexpr float kViewportWidth = 125.0F;
    static constexpr float kViewportHeight = 130.0F;

    static constexpr float kFeatureInitialY = 47.95F;
    static constexpr float kFeatureExpandedY = 38.0F;
    static constexpr float kFeatureCollapsedY = -150.0F;
    static constexpr float kFeatureAnimationSeconds = 0.5F;
    static constexpr Rect kFeatureToggleHit{-228.0F, 34.0F, -208.0F, 56.0F};

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
    SpriteTexture expand_;

    bool featureExpanded_ = true;
    bool featureVisible_ = true;
    bool featureAnimating_ = false;
    float featureY_ = kFeatureInitialY;
    float featureAnimationFromY_ = kFeatureInitialY;
    float featureAnimationTargetY_ = kFeatureInitialY;
    std::chrono::steady_clock::time_point featureAnimationStart_{};
};

} // namespace eudoria::game::ui
