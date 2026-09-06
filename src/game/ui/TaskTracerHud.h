#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/render/TextRasterizer.h"
#include "engine/ui/LegacyUiTransform.h"
#include "game/ui/HudWindowManager.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace eudoria::game::ui {

struct TrackedTask final {
    std::int32_t id = 0;
    std::int32_t kind = 0;
    std::wstring name;
    std::wstring condition;
    bool expanded = true;
};

struct AvailableTask final {
    std::int32_t id = 0;
    std::wstring name;
    std::wstring receiveAt;
};

class TaskTracerHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/task_tracer");

    void update();
    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    void setTrackedTasks(std::vector<TrackedTask> tasks);
    void setAvailableTasks(std::vector<AvailableTask> tasks);

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
        std::uint32_t viewportHeight,
        HudWindowManager& windows) noexcept;

    bool onMouseWheel(
        float mouseX,
        float mouseY,
        int wheelDelta,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    [[nodiscard]] bool expanded() const noexcept { return expanded_; }
    [[nodiscard]] bool dragging() const noexcept { return dragging_; }

private:
    enum class Mode : std::uint8_t {
        HaveTask,
        CanReceiveTask,
    };

    enum class PressedAction : std::uint8_t {
        None,
        Collapse,
        HaveTaskTab,
        CanReceiveTaskTab,
        ToggleTask,
        ViewTask,
        ScrollUp,
        ScrollDown,
        ScrollTrack,
        ScrollThumb,
    };

    struct Rect final {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;

        [[nodiscard]] bool contains(float x, float y) const noexcept {
            return x >= left && x <= right && y >= top && y <= bottom;
        }
    };

    struct TaskVisual final {
        std::int32_t id = 0;
        std::int32_t kind = 0;
        bool expanded = true;
        float y = 0.0F;
        float height = 0.0F;
        TextTextureResult name;
        TextTextureResult condition;
        TextTextureResult view;
    };

    struct ThumbGeometry final {
        float y = 0.0F;
        float height = 0.0F;
        bool visible = false;
    };

    [[nodiscard]] eudoria::ui::Point mappedRoot(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect titleRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect collapseRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect haveTaskTabRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect canReceiveTaskTabRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect contentRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect scrollUpRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect scrollDownRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect scrollTrackRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect scrollThumbRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect taskNameRect(
        const TaskVisual& task,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect taskViewRect(
        const TaskVisual& task,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] float scrollContentHeight() const noexcept;
    [[nodiscard]] bool scrollBarVisible() const noexcept;
    [[nodiscard]] ThumbGeometry thumbGeometry() const noexcept;

    void markDirty() noexcept { dirty_ = true; }
    void refreshVisuals();
    void rebuildTrackedTaskVisuals();
    void rebuildAvailableTaskVisual();
    void clampScroll() noexcept;
    void toggleTask(std::int32_t taskId);
    void pageScroll(float direction) noexcept;

    void renderChrome(SpriteRenderer& renderer, const eudoria::ui::Point& root, float scale) const;
    void renderScrollBar(SpriteRenderer& renderer, const eudoria::ui::Point& root, float scale) const;

    static void drawClipped(
        SpriteRenderer& renderer,
        const SpriteTexture& texture,
        float legacyX,
        float legacyY,
        float legacyWidth,
        float legacyHeight,
        float clipTopLegacy,
        float clipBottomLegacy,
        const eudoria::ui::Point& root,
        float scale);

    static constexpr eudoria::ui::Point kDefaultRoot{960.0F, 230.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopRight;

    // symbol4135 titleBox is character304 placed at (120,-18), scaleX 4.999313,
    // scaleY 0.750031. character303 bounds are exactly -24..24 in both axes,
    // producing the 240 x 36 legacy title chrome below.
    static constexpr float kTitleX = 0.0F;
    static constexpr float kTitleY = -36.0F;
    static constexpr float kTitleWidth = 240.0F;
    static constexpr float kTitleHeight = 36.0F;

    // DefineEditText 4133: placement (42,-33), bounds (-2..161,-2..15.65).
    static constexpr float kTitleTextX = 40.0F;
    static constexpr float kTitleTextY = -35.0F;
    static constexpr float kTitleTextWidth = 163.0F;
    static constexpr float kTitleTextHeight = 18.0F;

    // DefineEditText 4134 collapse glyph: placement (6,-23), bounds
    // (-2..14,-2..15.65). TaskTracerUI toggles the payload glyph ━ / ╋.
    static constexpr float kCollapseTextX = 4.0F;
    static constexpr float kCollapseTextY = -25.0F;
    static constexpr float kCollapseTextWidth = 16.0F;
    static constexpr float kCollapseTextHeight = 18.0F;

    // TableButton character436 instances recovered from symbol4135.
    static constexpr float kHaveTaskTabX = 32.1F;
    static constexpr float kCanReceiveTaskTabX = 122.0F;
    static constexpr float kTabY = -19.0F;
    static constexpr float kTabScaleX = 1.8317566F;
    static constexpr float kTabScaleY = 1.0315247F;
    static constexpr float kTabHitWidth = 93.0F;
    static constexpr float kTabHitHeight = 20.0F;

    // Task content / UIScrollBar geometry. UIScrollBar has WIDTH=15, arrows=14,
    // and scrollTask is scaled to a final 180 px height in symbol4135.
    static constexpr float kPointTaskX = 0.0F;
    static constexpr float kPointTaskY = 0.0F;
    static constexpr float kContentWidth = 224.0F;
    static constexpr float kContentHeight = 180.0F;
    static constexpr float kScrollX = 224.0F;
    static constexpr float kScrollY = 0.0F;
    static constexpr float kScrollWidth = 15.0F;
    static constexpr float kScrollHeight = 180.0F;
    static constexpr float kScrollArrowHeight = 14.0F;
    static constexpr float kScrollTrackY = 14.0F;
    static constexpr float kScrollTrackHeight = 152.0F;
    static constexpr float kScrollMinThumbHeight = 13.0F;

    // TaskTracerBoxUIMC symbol5665 uses a 1px separator at x=2.5, then three
    // text fields. The -2 bounds on DefineEditText are included here so text is
    // placed at the same raster origin as Flash, not merely at instance x/y.
    static constexpr float kTaskSeparatorX = 2.5F;
    static constexpr float kTaskSeparatorY = -0.5F;
    static constexpr float kTaskSeparatorWidth = 223.0F;
    static constexpr float kTaskSeparatorHeight = 1.0F;
    static constexpr float kTaskNameX = 3.0F;
    static constexpr float kTaskNameY = 3.0F;
    static constexpr float kTaskNameWidth = 168.0F;
    static constexpr float kTaskNameHeight = 29.0F;
    static constexpr float kTaskConditionX = 3.0F;
    static constexpr float kTaskConditionY = 31.7F;
    static constexpr float kTaskConditionWidth = 222.0F;
    static constexpr float kTaskConditionMaxHeight = 512.0F;
    static constexpr float kTaskViewX = 164.0F;
    static constexpr float kTaskViewY = 3.0F;
    static constexpr float kTaskViewWidth = 61.0F;
    static constexpr float kTaskViewHeight = 29.0F;

    SpriteRenderer* renderer_ = nullptr;
    SpriteTexture titleBox_;
    SpriteTexture tabNormal_;
    SpriteTexture tabActive_;
    SpriteTexture taskSeparator_;
    SpriteTexture scrollTrack_;
    SpriteTexture scrollUp_;
    SpriteTexture scrollDown_;
    SpriteTexture scrollThumb_;
    SpriteTexture scrollThumbIcon_;

    TextTextureResult titleLabel_;
    TextTextureResult collapseExpandedLabel_;
    TextTextureResult collapseCollapsedLabel_;
    TextTextureResult haveTaskLabel_;
    TextTextureResult canReceiveLabel_;

    std::vector<TrackedTask> trackedTasks_;
    std::vector<AvailableTask> availableTasks_;
    std::vector<TaskVisual> taskVisuals_;
    TextTextureResult availableText_;

    eudoria::ui::Point dragOffsetLegacy_{};
    eudoria::ui::Point dragStartMouse_{};
    eudoria::ui::Point dragStartOffset_{};
    Mode mode_ = Mode::HaveTask;
    PressedAction pressedAction_ = PressedAction::None;
    std::int32_t pressedTaskId_ = 0;
    float scrollPosition_ = 0.0F;
    float contentHeight_ = 0.0F;
    float scrollThumbGrabOffset_ = 0.0F;
    std::chrono::steady_clock::time_point lastRefresh_{};
    bool expanded_ = true;
    bool dragging_ = false;
    bool draggingScrollThumb_ = false;
    bool dirty_ = true;
};

} // namespace eudoria::game::ui
