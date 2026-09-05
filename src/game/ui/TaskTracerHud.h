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
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui");

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

    [[nodiscard]] eudoria::ui::Point mappedRoot(
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect titleRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect collapseRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect haveTaskTabRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect canReceiveTaskTabRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect contentRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect taskNameRect(
        const TaskVisual& task,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect taskViewRect(
        const TaskVisual& task,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) const noexcept;

    void markDirty() noexcept { dirty_ = true; }
    void refreshVisuals();
    void rebuildTrackedTaskVisuals();
    void rebuildAvailableTaskVisual();
    void clampScroll() noexcept;
    void toggleTask(std::int32_t taskId);

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

    static constexpr float kReferenceOriginX = 2.0F;
    static constexpr float kReferenceOriginY = 38.0F;
    static constexpr float kPayloadWidth = 242.0F;
    static constexpr float kPayloadHeight = 219.0F;
    static constexpr float kTitleWidth = 240.0F;
    static constexpr float kTitleHeight = 38.0F;
    static constexpr float kCollapseWidth = 31.0F;

    // Extracted directly from symbol4135 / TaskTracerUIMC.
    static constexpr float kPointTaskX = 0.0F;
    static constexpr float kPointTaskY = 0.0F;
    static constexpr float kScrollTaskX = 224.0F;
    static constexpr float kContentWidth = 224.0F;
    static constexpr float kContentHeight = 180.0F;
    static constexpr float kHaveTaskTabX = 32.1F;
    static constexpr float kCanReceiveTaskTabX = 122.0F;
    static constexpr float kTabY = -19.0F;
    static constexpr float kTabWidth = 88.0F;
    static constexpr float kTabHeight = 20.0F;

    // Extracted directly from symbol5665 / TaskTracerBoxUIMC and DefineEditText bounds.
    static constexpr float kTaskNameX = 5.0F;
    static constexpr float kTaskNameY = 5.0F;
    static constexpr float kTaskNameWidth = 168.0F;
    static constexpr float kTaskNameHeight = 29.0F;
    static constexpr float kTaskConditionX = 5.0F;
    static constexpr float kTaskConditionY = 33.7F;
    static constexpr float kTaskConditionWidth = 220.0F;
    static constexpr float kTaskConditionMaxHeight = 120.0F;
    static constexpr float kTaskViewX = 166.0F;
    static constexpr float kTaskViewY = 5.0F;
    static constexpr float kTaskViewWidth = 59.0F;
    static constexpr float kTaskViewHeight = 27.0F;

    SpriteRenderer* renderer_ = nullptr;
    SpriteTexture skin_;
    SpriteTexture taskBoxSkin_;
    TextTextureResult haveTaskLabelNormal_;
    TextTextureResult haveTaskLabelActive_;
    TextTextureResult canReceiveLabelNormal_;
    TextTextureResult canReceiveLabelActive_;
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
    std::chrono::steady_clock::time_point lastRefresh_{};
    bool expanded_ = true;
    bool dragging_ = false;
    bool dirty_ = true;
};

} // namespace eudoria::game::ui
