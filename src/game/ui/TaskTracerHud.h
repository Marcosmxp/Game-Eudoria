#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

class TaskTracerHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    void onViewportChanged() noexcept;

    bool onMouseDown(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    bool onMouseMove(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    bool onMouseUp(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    [[nodiscard]] bool expanded() const noexcept { return expanded_; }
    [[nodiscard]] bool dragging() const noexcept { return dragging_; }

private:
    struct Rect final {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;

        [[nodiscard]] bool contains(float x, float y) const noexcept {
            return x >= left && x <= right && y >= top && y <= bottom;
        }
    };

    [[nodiscard]] eudoria::ui::Point mappedRoot(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect titleRect(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect collapseRect(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    static constexpr eudoria::ui::Point kDefaultRoot{960.0F, 230.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopRight;

    // Original TaskTracerUI::onResize places the root at stageWidth-titleBox.width, y=230.
    // The exported UIMC raster has its Flash origin 2 px from the left and 38 px below
    // the top of the title chrome.
    static constexpr float kReferenceOriginX = 2.0F;
    static constexpr float kReferenceOriginY = 38.0F;
    static constexpr float kTitleWidth = 240.0F;
    static constexpr float kTitleHeight = 38.0F;
    static constexpr float kCollapseWidth = 31.0F;

    SpriteTexture skin_;
    eudoria::ui::Point dragOffsetLegacy_{};
    eudoria::ui::Point dragStartMouse_{};
    eudoria::ui::Point dragStartOffset_{};
    bool expanded_ = true;
    bool dragging_ = false;
    bool collapsePressed_ = false;
};

} // namespace eudoria::game::ui
