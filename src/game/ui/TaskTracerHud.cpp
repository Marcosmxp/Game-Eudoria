#include "game/ui/TaskTracerHud.h"

#include <algorithm>

namespace eudoria::game::ui {

bool TaskTracerHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot) {
    return renderer.loadTexture((referenceRoot / L"task_tracer.reference.png").wstring(), skin_);
}

void TaskTracerHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    if (!skin_.valid()) {
        return;
    }

    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const float x = root.x - (kReferenceOriginX * scale);
    const float y = root.y - (kReferenceOriginY * scale);

    // The exported raster is 530x442 because FFDec preserves the full symbol bounds,
    // but the first-frame TaskTracer chrome occupies only the payload bbox 242x219.
    // Rendering that region avoids a large transparent quad and gives us a clean
    // title-only crop when the ActionScript collapse control removes pointTask/scrollTask.
    constexpr float kPayloadWidth = 242.0F;
    constexpr float kPayloadHeight = 219.0F;

    if (expanded_) {
        renderer.drawRegion(
            skin_,
            SpriteSourceRect{0.0F, 0.0F, kPayloadWidth, kPayloadHeight},
            x,
            y,
            kPayloadWidth * scale,
            kPayloadHeight * scale);
        return;
    }

    renderer.drawRegion(
        skin_,
        SpriteSourceRect{0.0F, 0.0F, kPayloadWidth, kTitleHeight},
        x,
        y,
        kPayloadWidth * scale,
        kTitleHeight * scale);
}

void TaskTracerHud::onViewportChanged() noexcept {
    // TaskTracerUI::onResize() restores the original top-right location.
    dragOffsetLegacy_ = {};
    dragging_ = false;
    collapsePressed_ = false;
}

bool TaskTracerHud::onMouseDown(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        collapsePressed_ = true;
        dragging_ = false;
        return true;
    }

    if (!titleRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        return false;
    }

    dragStartMouse_ = {mouseX, mouseY};
    dragStartOffset_ = dragOffsetLegacy_;
    dragging_ = true;
    collapsePressed_ = false;
    return true;
}

bool TaskTracerHud::onMouseMove(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    if (!dragging_) {
        return false;
    }

    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = std::max(viewport.scale(), 0.0001F);
    dragOffsetLegacy_.x = dragStartOffset_.x + ((mouseX - dragStartMouse_.x) / scale);
    dragOffsetLegacy_.y = dragStartOffset_.y + ((mouseY - dragStartMouse_.y) / scale);
    return true;
}

bool TaskTracerHud::onMouseUp(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    bool consumed = false;

    if (collapsePressed_) {
        if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            expanded_ = !expanded_;
        }
        collapsePressed_ = false;
        consumed = true;
    }

    if (dragging_) {
        dragging_ = false;
        consumed = true;
    }

    return consumed;
}

eudoria::ui::Point TaskTracerHud::mappedRoot(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    return viewport.mapRoot(
        {
            kDefaultRoot.x + dragOffsetLegacy_.x,
            kDefaultRoot.y + dragOffsetLegacy_.y,
        },
        kAnchor);
}

TaskTracerHud::Rect TaskTracerHud::titleRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const float left = root.x - (kReferenceOriginX * scale);
    const float top = root.y - (kReferenceOriginY * scale);
    return {
        left,
        top,
        left + ((kTitleWidth + kReferenceOriginX) * scale),
        top + (kTitleHeight * scale),
    };
}

TaskTracerHud::Rect TaskTracerHud::collapseRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto title = titleRect(viewportWidth, viewportHeight);
    return {
        title.left,
        title.top,
        title.left + (kCollapseWidth * eudoria::ui::LegacyViewport{
            static_cast<float>(viewportWidth),
            static_cast<float>(viewportHeight),
        }.scale()),
        title.bottom,
    };
}

} // namespace eudoria::game::ui
