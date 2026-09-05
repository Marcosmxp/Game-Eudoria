#include "game/ui/TaskTracerHud.h"

#include <algorithm>
#include <utility>

namespace eudoria::game::ui {
namespace {
constexpr auto kRefreshInterval = std::chrono::milliseconds{2000};

TextTextureStyle textStyle(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b,
                           const TextHorizontalAlign align = TextHorizontalAlign::Left,
                           const bool wrap = true) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = 10;
    style.red = r;
    style.green = g;
    style.blue = b;
    style.align = align;
    style.wordWrap = wrap;
    return style;
}
} // namespace

bool TaskTracerHud::initialize(SpriteRenderer& renderer, const std::filesystem::path& referenceRoot) {
    renderer_ = &renderer;
    const bool loaded = renderer.loadTexture((referenceRoot / L"task_tracer.reference.png").wstring(), skin_);
    renderer.loadTexture((referenceRoot / L"task_tracer_box.reference.png").wstring(), taskBoxSkin_);

    createTextTexture(renderer, L"Current", 88, 18, textStyle(235, 235, 235, TextHorizontalAlign::Center, false), haveTaskLabelNormal_);
    createTextTexture(renderer, L"Current", 88, 18, textStyle(255, 204, 0, TextHorizontalAlign::Center, false), haveTaskLabelActive_);
    createTextTexture(renderer, L"Available", 88, 18, textStyle(235, 235, 235, TextHorizontalAlign::Center, false), canReceiveLabelNormal_);
    createTextTexture(renderer, L"Available", 88, 18, textStyle(255, 204, 0, TextHorizontalAlign::Center, false), canReceiveLabelActive_);

    lastRefresh_ = std::chrono::steady_clock::now() - kRefreshInterval;
    refreshVisuals();
    return loaded;
}

void TaskTracerHud::update() {
    const auto now = std::chrono::steady_clock::now();
    if (dirty_ || now - lastRefresh_ >= kRefreshInterval) {
        refreshVisuals();
        lastRefresh_ = now;
    }
}

void TaskTracerHud::setTrackedTasks(std::vector<TrackedTask> tasks) {
    trackedTasks_ = std::move(tasks);
    markDirty();
}

void TaskTracerHud::setAvailableTasks(std::vector<AvailableTask> tasks) {
    availableTasks_ = std::move(tasks);
    markDirty();
}

void TaskTracerHud::refreshVisuals() {
    if (!renderer_) return;
    std::sort(trackedTasks_.begin(), trackedTasks_.end(), [](const TrackedTask& a, const TrackedTask& b) {
        return a.kind == b.kind ? a.id < b.id : a.kind < b.kind;
    });
    rebuildTrackedTaskVisuals();
    rebuildAvailableTaskVisual();
    clampScroll();
    dirty_ = false;
}

void TaskTracerHud::rebuildTrackedTaskVisuals() {
    taskVisuals_.clear();
    contentHeight_ = 0.0F;
    if (!renderer_) return;

    for (const auto& task : trackedTasks_) {
        TaskVisual visual;
        visual.id = task.id;
        visual.kind = task.kind;
        visual.expanded = task.expanded;
        visual.y = contentHeight_ + 3.0F;

        createTextTexture(*renderer_, (task.expanded ? L"－" : L"＋") + task.name,
            static_cast<std::uint32_t>(kTaskNameWidth), static_cast<std::uint32_t>(kTaskNameHeight),
            textStyle(255, 204, 0), visual.name);
        createTextTexture(*renderer_, L"View Task",
            static_cast<std::uint32_t>(kTaskViewWidth), static_cast<std::uint32_t>(kTaskViewHeight),
            textStyle(204, 255, 0, TextHorizontalAlign::Right, false), visual.view);

        float conditionHeight = 0.0F;
        if (task.expanded && !task.condition.empty()) {
            createTextTexture(*renderer_, task.condition,
                static_cast<std::uint32_t>(kTaskConditionWidth), static_cast<std::uint32_t>(kTaskConditionMaxHeight),
                textStyle(255, 255, 255), visual.condition);
            conditionHeight = static_cast<float>(visual.condition.contentHeight);
        }

        visual.height = task.expanded
            ? std::max(kTaskConditionY + conditionHeight + 3.0F, kTaskNameY + kTaskNameHeight)
            : kTaskNameY + kTaskNameHeight;
        contentHeight_ += visual.height;
        taskVisuals_.push_back(std::move(visual));
    }
}

void TaskTracerHud::rebuildAvailableTaskVisual() {
    availableText_ = {};
    if (!renderer_ || availableTasks_.empty()) return;

    std::wstring text;
    for (const auto& task : availableTasks_) {
        if (!text.empty()) text += L"\n";
        text += L"---------\n" + task.name;
        if (!task.receiveAt.empty()) text += L"\n" + task.receiveAt;
    }
    createTextTexture(*renderer_, text, static_cast<std::uint32_t>(kContentWidth - 4.0F), 2048,
        textStyle(255, 255, 255), availableText_);
}

void TaskTracerHud::render(SpriteRenderer& renderer, const std::uint32_t viewportWidth,
                           const std::uint32_t viewportHeight) const {
    if (!skin_.valid()) return;
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const float skinX = root.x - kReferenceOriginX * scale;
    const float skinY = root.y - kReferenceOriginY * scale;

    if (!expanded_) {
        renderer.drawRegion(skin_, {0.0F, 0.0F, kPayloadWidth, kTitleHeight}, skinX, skinY,
            kPayloadWidth * scale, kTitleHeight * scale);
        return;
    }

    renderer.drawRegion(skin_, {0.0F, 0.0F, kPayloadWidth, kPayloadHeight}, skinX, skinY,
        kPayloadWidth * scale, kPayloadHeight * scale);

    const auto& haveLabel = mode_ == Mode::HaveTask ? haveTaskLabelActive_ : haveTaskLabelNormal_;
    const auto& availableLabel = mode_ == Mode::CanReceiveTask ? canReceiveLabelActive_ : canReceiveLabelNormal_;
    if (haveLabel.texture.valid()) {
        renderer.draw(haveLabel.texture, root.x + kHaveTaskTabX * scale, root.y + kTabY * scale,
            kTabWidth * scale, static_cast<float>(haveLabel.texture.height) * scale);
    }
    if (availableLabel.texture.valid()) {
        renderer.draw(availableLabel.texture, root.x + kCanReceiveTaskTabX * scale, root.y + kTabY * scale,
            kTabWidth * scale, static_cast<float>(availableLabel.texture.height) * scale);
    }

    constexpr float clipTop = kPointTaskY;
    constexpr float clipBottom = kPointTaskY + kContentHeight;
    if (mode_ == Mode::CanReceiveTask) {
        if (availableText_.texture.valid()) {
            drawClipped(renderer, availableText_.texture, kPointTaskX + 2.0F,
                kPointTaskY + 2.0F - scrollPosition_, static_cast<float>(availableText_.texture.width),
                static_cast<float>(availableText_.texture.height), clipTop, clipBottom, root, scale);
        }
        return;
    }

    for (const auto& task : taskVisuals_) {
        const float taskY = task.y - scrollPosition_;
        if (taskY >= clipBottom || taskY + task.height <= clipTop) continue;

        if (taskBoxSkin_.valid()) {
            constexpr float separatorWidth = 160.0F;
            constexpr float separatorHeight = 18.0F;
            const float visibleTop = std::max(taskY, clipTop);
            const float visibleBottom = std::min(taskY + separatorHeight, clipBottom);
            if (visibleBottom > visibleTop) {
                const float cropTop = visibleTop - taskY;
                const float visibleHeight = visibleBottom - visibleTop;
                renderer.drawRegion(taskBoxSkin_, {0.0F, cropTop, separatorWidth, visibleHeight},
                    root.x, root.y + visibleTop * scale, separatorWidth * scale, visibleHeight * scale);
            }
        }

        if (task.name.texture.valid()) {
            drawClipped(renderer, task.name.texture, kTaskNameX, taskY + kTaskNameY,
                static_cast<float>(task.name.texture.width), static_cast<float>(task.name.texture.height),
                clipTop, clipBottom, root, scale);
        }
        if (task.view.texture.valid()) {
            drawClipped(renderer, task.view.texture, kTaskViewX, taskY + kTaskViewY,
                static_cast<float>(task.view.texture.width), static_cast<float>(task.view.texture.height),
                clipTop, clipBottom, root, scale);
        }
        if (task.expanded && task.condition.texture.valid()) {
            drawClipped(renderer, task.condition.texture, kTaskConditionX, taskY + kTaskConditionY,
                static_cast<float>(task.condition.texture.width), static_cast<float>(task.condition.texture.height),
                clipTop, clipBottom, root, scale);
        }
    }
}

void TaskTracerHud::drawClipped(SpriteRenderer& renderer, const SpriteTexture& texture,
                                const float legacyX, const float legacyY, const float legacyWidth,
                                const float legacyHeight, const float clipTopLegacy,
                                const float clipBottomLegacy, const eudoria::ui::Point& root,
                                const float scale) {
    if (!texture.valid() || legacyWidth <= 0.0F || legacyHeight <= 0.0F) return;
    const float top = std::max(legacyY, clipTopLegacy);
    const float bottom = std::min(legacyY + legacyHeight, clipBottomLegacy);
    if (bottom <= top) return;
    const float crop = top - legacyY;
    const float visible = bottom - top;
    renderer.drawRegion(texture,
        {0.0F, crop * static_cast<float>(texture.height) / legacyHeight,
         static_cast<float>(texture.width), visible * static_cast<float>(texture.height) / legacyHeight},
        root.x + legacyX * scale, root.y + top * scale, legacyWidth * scale, visible * scale);
}

void TaskTracerHud::onViewportChanged() noexcept {
    dragOffsetLegacy_ = {};
    dragging_ = false;
    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;
}

bool TaskTracerHud::onMouseDown(const float mouseX, const float mouseY, const std::uint32_t viewportWidth,
                                const std::uint32_t viewportHeight) noexcept {
    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;

    if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::Collapse;
        dragging_ = false;
        return true;
    }
    if (expanded_ && haveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::HaveTaskTab;
        return true;
    }
    if (expanded_ && canReceiveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::CanReceiveTaskTab;
        return true;
    }

    if (expanded_ && mode_ == Mode::HaveTask && contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        for (const auto& task : taskVisuals_) {
            if (taskViewRect(task, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
                pressedAction_ = PressedAction::ViewTask;
                pressedTaskId_ = task.id;
                return true;
            }
            if (taskNameRect(task, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
                pressedAction_ = PressedAction::ToggleTask;
                pressedTaskId_ = task.id;
                return true;
            }
        }
    }

    if (!titleRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) return false;
    dragStartMouse_ = {mouseX, mouseY};
    dragStartOffset_ = dragOffsetLegacy_;
    dragging_ = true;
    return true;
}

bool TaskTracerHud::onMouseMove(const float mouseX, const float mouseY, const std::uint32_t viewportWidth,
                                const std::uint32_t viewportHeight) noexcept {
    if (!dragging_) return false;
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = std::max(viewport.scale(), 0.0001F);
    dragOffsetLegacy_.x = dragStartOffset_.x + (mouseX - dragStartMouse_.x) / scale;
    dragOffsetLegacy_.y = dragStartOffset_.y + (mouseY - dragStartMouse_.y) / scale;
    return true;
}

bool TaskTracerHud::onMouseUp(const float mouseX, const float mouseY, const std::uint32_t viewportWidth,
                              const std::uint32_t viewportHeight, HudWindowManager& windows) noexcept {
    bool consumed = false;
    switch (pressedAction_) {
    case PressedAction::Collapse:
        if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) expanded_ = !expanded_;
        consumed = true;
        break;
    case PressedAction::HaveTaskTab:
        if (haveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            mode_ = Mode::HaveTask;
            scrollPosition_ = 0.0F;
            clampScroll();
        }
        consumed = true;
        break;
    case PressedAction::CanReceiveTaskTab:
        if (canReceiveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            mode_ = Mode::CanReceiveTask;
            scrollPosition_ = 0.0F;
            clampScroll();
        }
        consumed = true;
        break;
    case PressedAction::ToggleTask:
        if (pressedTaskId_ != 0) toggleTask(pressedTaskId_);
        consumed = true;
        break;
    case PressedAction::ViewTask: {
        const auto it = std::find_if(taskVisuals_.begin(), taskVisuals_.end(),
            [this](const TaskVisual& task) { return task.id == pressedTaskId_; });
        if (it != taskVisuals_.end() && taskViewRect(*it, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            windows.show(HudWindow::Quests);
        }
        consumed = true;
        break;
    }
    case PressedAction::None:
        break;
    }
    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;
    if (dragging_) {
        dragging_ = false;
        consumed = true;
    }
    return consumed;
}

bool TaskTracerHud::onMouseWheel(const float mouseX, const float mouseY, const int wheelDelta,
                                 const std::uint32_t viewportWidth, const std::uint32_t viewportHeight) noexcept {
    if (!expanded_ || !contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) return false;
    scrollPosition_ -= (static_cast<float>(wheelDelta) / 120.0F) * 24.0F;
    clampScroll();
    return true;
}

void TaskTracerHud::toggleTask(const std::int32_t taskId) {
    const auto it = std::find_if(trackedTasks_.begin(), trackedTasks_.end(),
        [taskId](const TrackedTask& task) { return task.id == taskId; });
    if (it == trackedTasks_.end()) return;
    it->expanded = !it->expanded;
    markDirty();
    refreshVisuals();
}

void TaskTracerHud::clampScroll() noexcept {
    float total = 0.0F;
    if (mode_ == Mode::HaveTask) total = contentHeight_;
    else if (availableText_.texture.valid()) total = static_cast<float>(availableText_.texture.height);
    scrollPosition_ = std::clamp(scrollPosition_, 0.0F, std::max(0.0F, total - kContentHeight));
}

eudoria::ui::Point TaskTracerHud::mappedRoot(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(w), static_cast<float>(h)};
    return viewport.mapRoot({kDefaultRoot.x + dragOffsetLegacy_.x, kDefaultRoot.y + dragOffsetLegacy_.y}, kAnchor);
}

TaskTracerHud::Rect TaskTracerHud::titleRect(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale();
    const auto r = mappedRoot(w, h);
    const float left = r.x - kReferenceOriginX * s;
    const float top = r.y - kReferenceOriginY * s;
    return {left, top, left + (kTitleWidth + kReferenceOriginX) * s, top + kTitleHeight * s};
}

TaskTracerHud::Rect TaskTracerHud::collapseRect(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const auto t = titleRect(w, h);
    const float s = eudoria::ui::LegacyViewport{static_cast<float>(w), static_cast<float>(h)}.scale();
    return {t.left, t.top, t.left + kCollapseWidth * s, t.bottom};
}

TaskTracerHud::Rect TaskTracerHud::haveTaskTabRect(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale(); const auto r = mappedRoot(w, h);
    return {r.x + kHaveTaskTabX * s, r.y + kTabY * s, r.x + (kHaveTaskTabX + kTabWidth) * s,
            r.y + (kTabY + kTabHeight) * s};
}

TaskTracerHud::Rect TaskTracerHud::canReceiveTaskTabRect(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale(); const auto r = mappedRoot(w, h);
    return {r.x + kCanReceiveTaskTabX * s, r.y + kTabY * s, r.x + (kCanReceiveTaskTabX + kTabWidth) * s,
            r.y + (kTabY + kTabHeight) * s};
}

TaskTracerHud::Rect TaskTracerHud::contentRect(const std::uint32_t w, const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale(); const auto r = mappedRoot(w, h);
    return {r.x, r.y, r.x + kContentWidth * s, r.y + kContentHeight * s};
}

TaskTracerHud::Rect TaskTracerHud::taskNameRect(const TaskVisual& task, const std::uint32_t w,
                                                const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale(); const auto r = mappedRoot(w, h);
    const float y = task.y - scrollPosition_ + kTaskNameY;
    return {r.x + kTaskNameX * s, r.y + y * s, r.x + (kTaskNameX + kTaskNameWidth) * s,
            r.y + (y + kTaskNameHeight) * s};
}

TaskTracerHud::Rect TaskTracerHud::taskViewRect(const TaskVisual& task, const std::uint32_t w,
                                                const std::uint32_t h) const noexcept {
    const eudoria::ui::LegacyViewport v{static_cast<float>(w), static_cast<float>(h)};
    const float s = v.scale(); const auto r = mappedRoot(w, h);
    const float y = task.y - scrollPosition_ + kTaskViewY;
    return {r.x + kTaskViewX * s, r.y + y * s, r.x + (kTaskViewX + kTaskViewWidth) * s,
            r.y + (y + kTaskViewHeight) * s};
}

} // namespace eudoria::game::ui
