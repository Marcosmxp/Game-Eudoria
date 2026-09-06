#include "game/ui/TaskTracerHud.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace eudoria::game::ui {
namespace {
constexpr auto kRefreshInterval = std::chrono::milliseconds{2000};

TextTextureStyle textStyle(
    const std::uint8_t r,
    const std::uint8_t g,
    const std::uint8_t b,
    const std::uint32_t size = 10,
    const TextHorizontalAlign align = TextHorizontalAlign::Left,
    const bool wrap = true) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = size;
    style.red = r;
    style.green = g;
    style.blue = b;
    style.align = align;
    style.wordWrap = wrap;
    return style;
}
} // namespace

bool TaskTracerHud::initialize(SpriteRenderer& renderer, const std::filesystem::path& runtimeRoot) {
    renderer_ = &renderer;
    bool loadedAny = false;

    const auto load = [&](const std::filesystem::path& relative, SpriteTexture& texture) {
        const bool loaded = renderer.loadTexture((runtimeRoot / relative).wstring(), texture);
        loadedAny = loadedAny || loaded;
        return loaded;
    };

    // All of these are direct character/shape exports from assets.swf. The
    // symbol4135 FFDec composite is intentionally not used by runtime anymore.
    load(L"title_box.png", titleBox_);             // shape303 / character304
    load(L"tab/normal.png", tabNormal_);          // shape432 / TableButton frame 1
    load(L"tab/active.png", tabActive_);          // shape435 / TableButton frame 2
    load(L"task_separator.png", taskSeparator_);  // shape5661 / TaskTracerBoxUIMC
    load(L"scroll/track.png", scrollTrack_);      // ScrollTrack_skin / shape191
    load(L"scroll/up.png", scrollUp_);            // ScrollArrowUp_upSkin / shape194
    load(L"scroll/down.png", scrollDown_);        // ScrollArrowDown_upSkin / shape196
    load(L"scroll/thumb.png", scrollThumb_);      // ScrollThumb_upSkin / shape198
    load(L"scroll/thumb_icon.png", scrollThumbIcon_); // ScrollBar_thumbIcon / shape208

    // Exact English localization recovered from txt/idc.json -> TaskTracerUI.
    createTextTexture(
        renderer,
        L"Quest Tracking",
        static_cast<std::uint32_t>(kTitleTextWidth),
        static_cast<std::uint32_t>(kTitleTextHeight),
        textStyle(255, 255, 255, 12, TextHorizontalAlign::Center, false),
        titleLabel_);
    createTextTexture(
        renderer,
        L"━",
        static_cast<std::uint32_t>(kCollapseTextWidth),
        static_cast<std::uint32_t>(kCollapseTextHeight),
        textStyle(255, 255, 255, 12, TextHorizontalAlign::Center, false),
        collapseExpandedLabel_);
    createTextTexture(
        renderer,
        L"╋",
        static_cast<std::uint32_t>(kCollapseTextWidth),
        static_cast<std::uint32_t>(kCollapseTextHeight),
        textStyle(255, 255, 255, 12, TextHorizontalAlign::Center, false),
        collapseCollapsedLabel_);
    createTextTexture(
        renderer,
        L"Current",
        static_cast<std::uint32_t>(kTabHitWidth),
        18,
        textStyle(255, 255, 255, 10, TextHorizontalAlign::Center, false),
        haveTaskLabel_);
    createTextTexture(
        renderer,
        L"Available",
        static_cast<std::uint32_t>(kTabHitWidth),
        18,
        textStyle(255, 255, 255, 10, TextHorizontalAlign::Center, false),
        canReceiveLabel_);

    lastRefresh_ = std::chrono::steady_clock::now() - kRefreshInterval;
    refreshVisuals();
    return loadedAny;
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
    if (!renderer_) {
        return;
    }

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
    if (!renderer_) {
        return;
    }

    for (const auto& task : trackedTasks_) {
        TaskVisual visual;
        visual.id = task.id;
        visual.kind = task.kind;
        visual.expanded = task.expanded;
        visual.y = contentHeight_ + 3.0F;

        // TaskTracerBoxUI.refreshTask() uses these exact full-width glyphs.
        createTextTexture(
            *renderer_,
            (task.expanded ? L"－" : L"＋") + task.name,
            static_cast<std::uint32_t>(kTaskNameWidth),
            static_cast<std::uint32_t>(kTaskNameHeight),
            textStyle(255, 204, 0, 10),
            visual.name);

        // txtClickShowContent in the real localization catalog is "More Info".
        createTextTexture(
            *renderer_,
            L"More Info",
            static_cast<std::uint32_t>(kTaskViewWidth),
            static_cast<std::uint32_t>(kTaskViewHeight),
            textStyle(204, 255, 0, 10, TextHorizontalAlign::Right, false),
            visual.view);

        float conditionFieldHeight = 0.0F;
        if (task.expanded && !task.condition.empty()) {
            createTextTexture(
                *renderer_,
                task.condition,
                static_cast<std::uint32_t>(kTaskConditionWidth),
                static_cast<std::uint32_t>(kTaskConditionMaxHeight),
                textStyle(255, 255, 255, 10),
                visual.condition);

            // TaskTracerBoxUI sets txtTaskCondition.height = textHeight + 5.
            conditionFieldHeight = static_cast<float>(visual.condition.contentHeight) + 5.0F;
        }

        const float headerBottom = std::max(
            kTaskNameY + kTaskNameHeight,
            kTaskViewY + kTaskViewHeight);
        visual.height = task.expanded && conditionFieldHeight > 0.0F
            ? std::max(headerBottom, kTaskConditionY + conditionFieldHeight)
            : headerBottom;

        contentHeight_ += visual.height;
        taskVisuals_.push_back(std::move(visual));
    }
}

void TaskTracerHud::rebuildAvailableTaskVisual() {
    availableText_ = {};
    if (!renderer_ || availableTasks_.empty()) {
        return;
    }

    // TaskTracerUI.showCanReceiveTask() builds exactly this text structure.
    std::wstring text;
    for (const auto& task : availableTasks_) {
        text += L"\n---------";
        text += L"\n" + task.name;
        if (!task.receiveAt.empty()) {
            text += L"\n" + task.receiveAt;
        }
    }

    createTextTexture(
        *renderer_,
        text,
        224,
        2048,
        textStyle(255, 255, 255, 12),
        availableText_);
}

void TaskTracerHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(viewportWidth, viewportHeight);

    renderChrome(renderer, root, scale);

    constexpr float clipTop = kPointTaskY;
    constexpr float clipBottom = kPointTaskY + kContentHeight;

    // The collapse control in TaskTracerUI removes pointTask + scrollTask only.
    // Available-task text and its bound UIScrollBar are separate children and
    // therefore remain independent, just as in the legacy ActionScript.
    if (mode_ == Mode::CanReceiveTask) {
        if (availableText_.texture.valid()) {
            drawClipped(
                renderer,
                availableText_.texture,
                kPointTaskX,
                kPointTaskY - scrollPosition_,
                static_cast<float>(availableText_.texture.width),
                static_cast<float>(availableText_.texture.height),
                clipTop,
                clipBottom,
                root,
                scale);
        }
        renderScrollBar(renderer, root, scale);
        return;
    }

    if (!expanded_) {
        return;
    }

    for (const auto& task : taskVisuals_) {
        const float taskY = task.y - scrollPosition_;
        if (taskY >= clipBottom || taskY + task.height <= clipTop) {
            continue;
        }

        if (taskSeparator_.valid()) {
            const float separatorY = taskY + kTaskSeparatorY;
            const float visibleTop = std::max(separatorY, clipTop);
            const float visibleBottom = std::min(separatorY + kTaskSeparatorHeight, clipBottom);
            if (visibleBottom > visibleTop) {
                renderer.drawRegion(
                    taskSeparator_,
                    {
                        0.0F,
                        visibleTop - separatorY,
                        static_cast<float>(taskSeparator_.width),
                        visibleBottom - visibleTop,
                    },
                    root.x + kTaskSeparatorX * scale,
                    root.y + visibleTop * scale,
                    kTaskSeparatorWidth * scale,
                    (visibleBottom - visibleTop) * scale);
            }
        }

        if (task.name.texture.valid()) {
            drawClipped(
                renderer,
                task.name.texture,
                kTaskNameX,
                taskY + kTaskNameY,
                static_cast<float>(task.name.texture.width),
                static_cast<float>(task.name.texture.height),
                clipTop,
                clipBottom,
                root,
                scale);
        }

        if (task.view.texture.valid()) {
            drawClipped(
                renderer,
                task.view.texture,
                kTaskViewX,
                taskY + kTaskViewY,
                static_cast<float>(task.view.texture.width),
                static_cast<float>(task.view.texture.height),
                clipTop,
                clipBottom,
                root,
                scale);
        }

        if (task.expanded && task.condition.texture.valid()) {
            drawClipped(
                renderer,
                task.condition.texture,
                kTaskConditionX,
                taskY + kTaskConditionY,
                static_cast<float>(task.condition.texture.width),
                static_cast<float>(task.condition.texture.height),
                clipTop,
                clipBottom,
                root,
                scale);
        }
    }

    renderScrollBar(renderer, root, scale);
}

void TaskTracerHud::renderChrome(
    SpriteRenderer& renderer,
    const eudoria::ui::Point& root,
    const float scale) const {
    if (titleBox_.valid()) {
        renderer.draw(
            titleBox_,
            root.x + kTitleX * scale,
            root.y + kTitleY * scale,
            kTitleWidth * scale,
            kTitleHeight * scale);
    }

    if (titleLabel_.texture.valid()) {
        renderer.draw(
            titleLabel_.texture,
            root.x + kTitleTextX * scale,
            root.y + kTitleTextY * scale,
            kTitleTextWidth * scale,
            static_cast<float>(titleLabel_.texture.height) * scale);
    }

    const auto& collapse = expanded_ ? collapseExpandedLabel_ : collapseCollapsedLabel_;
    if (collapse.texture.valid()) {
        renderer.draw(
            collapse.texture,
            root.x + kCollapseTextX * scale,
            root.y + kCollapseTextY * scale,
            kCollapseTextWidth * scale,
            static_cast<float>(collapse.texture.height) * scale);
    }

    const auto drawTab = [&](const bool active, const float x, const TextTextureResult& label) {
        const auto& texture = active ? tabActive_ : tabNormal_;
        if (texture.valid()) {
            // TableButton frame 1 uses shape432 bounds (1..50.1,1..18), while
            // frame 2 uses shape435 bounds (0..50.75,0..18). These offsets are
            // applied after the symbol4135 instance transform.
            const float boundsX = active ? 0.0F : 1.0F;
            const float boundsY = active ? 0.0F : 1.0F;
            renderer.draw(
                texture,
                root.x + (x + boundsX * kTabScaleX) * scale,
                root.y + (kTabY + boundsY * kTabScaleY) * scale,
                static_cast<float>(texture.width) * kTabScaleX * scale,
                static_cast<float>(texture.height) * kTabScaleY * scale);
        }
        if (label.texture.valid()) {
            renderer.draw(
                label.texture,
                root.x + x * scale,
                root.y + (kTabY + 2.0F) * scale,
                kTabHitWidth * scale,
                static_cast<float>(label.texture.height) * scale);
        }
    };

    drawTab(mode_ == Mode::HaveTask, kHaveTaskTabX, haveTaskLabel_);
    drawTab(mode_ == Mode::CanReceiveTask, kCanReceiveTaskTabX, canReceiveLabel_);
}

void TaskTracerHud::renderScrollBar(
    SpriteRenderer& renderer,
    const eudoria::ui::Point& root,
    const float scale) const {
    if (!scrollBarVisible()) {
        return;
    }

    if (scrollTrack_.valid()) {
        renderer.draw(
            scrollTrack_,
            root.x + kScrollX * scale,
            root.y + kScrollTrackY * scale,
            kScrollWidth * scale,
            kScrollTrackHeight * scale);
    }
    if (scrollUp_.valid()) {
        renderer.draw(
            scrollUp_,
            root.x + kScrollX * scale,
            root.y + kScrollY * scale,
            kScrollWidth * scale,
            kScrollArrowHeight * scale);
    }
    if (scrollDown_.valid()) {
        renderer.draw(
            scrollDown_,
            root.x + kScrollX * scale,
            root.y + (kScrollHeight - kScrollArrowHeight) * scale,
            kScrollWidth * scale,
            kScrollArrowHeight * scale);
    }

    const auto thumb = thumbGeometry();
    if (!thumb.visible) {
        return;
    }

    if (scrollThumb_.valid()) {
        renderer.draw(
            scrollThumb_,
            root.x + kScrollX * scale,
            root.y + thumb.y * scale,
            kScrollWidth * scale,
            thumb.height * scale);
    }
    if (scrollThumbIcon_.valid() && thumb.height >= 7.0F) {
        renderer.draw(
            scrollThumbIcon_,
            root.x + (kScrollX + 5.0F) * scale,
            root.y + (thumb.y + (thumb.height - 7.0F) * 0.5F) * scale,
            5.0F * scale,
            7.0F * scale);
    }
}

void TaskTracerHud::drawClipped(
    SpriteRenderer& renderer,
    const SpriteTexture& texture,
    const float legacyX,
    const float legacyY,
    const float legacyWidth,
    const float legacyHeight,
    const float clipTopLegacy,
    const float clipBottomLegacy,
    const eudoria::ui::Point& root,
    const float scale) {
    if (!texture.valid() || legacyWidth <= 0.0F || legacyHeight <= 0.0F) {
        return;
    }

    const float top = std::max(legacyY, clipTopLegacy);
    const float bottom = std::min(legacyY + legacyHeight, clipBottomLegacy);
    if (bottom <= top) {
        return;
    }

    const float crop = top - legacyY;
    const float visible = bottom - top;
    renderer.drawRegion(
        texture,
        {
            0.0F,
            crop * static_cast<float>(texture.height) / legacyHeight,
            static_cast<float>(texture.width),
            visible * static_cast<float>(texture.height) / legacyHeight,
        },
        root.x + legacyX * scale,
        root.y + top * scale,
        legacyWidth * scale,
        visible * scale);
}

void TaskTracerHud::onViewportChanged() noexcept {
    dragOffsetLegacy_ = {};
    dragging_ = false;
    draggingScrollThumb_ = false;
    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;
}

bool TaskTracerHud::onMouseDown(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;

    if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::Collapse;
        dragging_ = false;
        draggingScrollThumb_ = false;
        return true;
    }

    if (haveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::HaveTaskTab;
        return true;
    }
    if (canReceiveTaskTabRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::CanReceiveTaskTab;
        return true;
    }

    if (scrollBarVisible()) {
        const auto thumb = thumbGeometry();
        if (thumb.visible && scrollThumbRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            const auto root = mappedRoot(viewportWidth, viewportHeight);
            const float scale = std::max(
                eudoria::ui::LegacyViewport{
                    static_cast<float>(viewportWidth),
                    static_cast<float>(viewportHeight)}.scale(),
                0.0001F);
            const float mouseLegacyY = (mouseY - root.y) / scale;
            scrollThumbGrabOffset_ = mouseLegacyY - thumb.y;
            draggingScrollThumb_ = true;
            pressedAction_ = PressedAction::ScrollThumb;
            return true;
        }
        if (scrollUpRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::ScrollUp;
            return true;
        }
        if (scrollDownRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::ScrollDown;
            return true;
        }
        if (scrollTrackRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::ScrollTrack;
            return true;
        }
    }

    if (expanded_ && mode_ == Mode::HaveTask &&
        contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
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

    if (!titleRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        return false;
    }

    dragStartMouse_ = {mouseX, mouseY};
    dragStartOffset_ = dragOffsetLegacy_;
    dragging_ = true;
    return true;
}

bool TaskTracerHud::onMouseMove(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = std::max(viewport.scale(), 0.0001F);

    if (draggingScrollThumb_) {
        const auto geometry = thumbGeometry();
        const float range = kScrollTrackHeight - geometry.height;
        const float maxScroll = std::max(0.0F, scrollContentHeight() - kContentHeight);
        if (geometry.visible && range > 0.0F && maxScroll > 0.0F) {
            const auto root = mappedRoot(viewportWidth, viewportHeight);
            const float mouseLegacyY = (mouseY - root.y) / scale;
            const float thumbY = std::clamp(
                mouseLegacyY - scrollThumbGrabOffset_,
                kScrollTrackY,
                kScrollTrackY + range);
            scrollPosition_ = ((thumbY - kScrollTrackY) / range) * maxScroll;
            clampScroll();
        }
        return true;
    }

    if (!dragging_) {
        return false;
    }

    dragOffsetLegacy_.x = dragStartOffset_.x + (mouseX - dragStartMouse_.x) / scale;
    dragOffsetLegacy_.y = dragStartOffset_.y + (mouseY - dragStartMouse_.y) / scale;
    return true;
}

bool TaskTracerHud::onMouseUp(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    HudWindowManager& windows) noexcept {
    bool consumed = false;

    switch (pressedAction_) {
    case PressedAction::Collapse:
        if (collapseRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            expanded_ = !expanded_;
            if (!expanded_ && mode_ == Mode::HaveTask) {
                scrollPosition_ = 0.0F;
            }
        }
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
        if (pressedTaskId_ != 0) {
            toggleTask(pressedTaskId_);
        }
        consumed = true;
        break;

    case PressedAction::ViewTask: {
        const auto it = std::find_if(
            taskVisuals_.begin(),
            taskVisuals_.end(),
            [this](const TaskVisual& task) { return task.id == pressedTaskId_; });
        if (it != taskVisuals_.end() &&
            taskViewRect(*it, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            windows.show(HudWindow::Quests);
        }
        consumed = true;
        break;
    }

    case PressedAction::ScrollUp:
        if (scrollUpRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            scrollPosition_ -= 1.0F; // ScrollBar._lineScrollSize default from payload.
            clampScroll();
        }
        consumed = true;
        break;

    case PressedAction::ScrollDown:
        if (scrollDownRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            scrollPosition_ += 1.0F;
            clampScroll();
        }
        consumed = true;
        break;

    case PressedAction::ScrollTrack:
        if (scrollTrackRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            const auto thumb = scrollThumbRect(viewportWidth, viewportHeight);
            if (thumb.bottom > thumb.top) {
                pageScroll(mouseY < thumb.top ? -1.0F : 1.0F);
            }
        }
        consumed = true;
        break;

    case PressedAction::ScrollThumb:
        draggingScrollThumb_ = false;
        consumed = true;
        break;

    case PressedAction::None:
        break;
    }

    pressedAction_ = PressedAction::None;
    pressedTaskId_ = 0;

    if (dragging_) {
        dragging_ = false;
        consumed = true;
    }
    if (draggingScrollThumb_) {
        draggingScrollThumb_ = false;
        consumed = true;
    }

    return consumed;
}

bool TaskTracerHud::onMouseWheel(
    const float mouseX,
    const float mouseY,
    const int wheelDelta,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    const bool overContent = contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
    const bool overScroll = scrollTrackRect(viewportWidth, viewportHeight).contains(mouseX, mouseY) ||
        scrollUpRect(viewportWidth, viewportHeight).contains(mouseX, mouseY) ||
        scrollDownRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);

    if ((!expanded_ && mode_ == Mode::HaveTask) || (!overContent && !overScroll)) {
        return false;
    }

    scrollPosition_ -= (static_cast<float>(wheelDelta) / 120.0F) * 24.0F;
    clampScroll();
    return true;
}

void TaskTracerHud::toggleTask(const std::int32_t taskId) {
    const auto it = std::find_if(
        trackedTasks_.begin(),
        trackedTasks_.end(),
        [taskId](const TrackedTask& task) { return task.id == taskId; });
    if (it == trackedTasks_.end()) {
        return;
    }

    it->expanded = !it->expanded;
    markDirty();
    refreshVisuals();
}

void TaskTracerHud::pageScroll(const float direction) noexcept {
    scrollPosition_ += direction * kContentHeight;
    clampScroll();
}

float TaskTracerHud::scrollContentHeight() const noexcept {
    if (mode_ == Mode::HaveTask) {
        return contentHeight_;
    }
    return availableText_.texture.valid()
        ? static_cast<float>(availableText_.contentHeight)
        : 0.0F;
}

bool TaskTracerHud::scrollBarVisible() const noexcept {
    if (mode_ == Mode::CanReceiveTask) {
        // showCanReceiveTask() explicitly sets the text-bound scrollbar visible.
        return true;
    }
    return expanded_ && contentHeight_ > kContentHeight;
}

TaskTracerHud::ThumbGeometry TaskTracerHud::thumbGeometry() const noexcept {
    ThumbGeometry result;
    const float total = scrollContentHeight();
    const float maxScroll = std::max(0.0F, total - kContentHeight);
    if (total <= 0.0F || maxScroll <= 0.0F || kScrollTrackHeight <= 12.0F) {
        return result;
    }

    // ScrollBar.updateThumb() from the payload:
    // max(13, pageSize / (max-min+pageSize) * track.height)
    result.height = std::max(
        kScrollMinThumbHeight,
        (kContentHeight / total) * kScrollTrackHeight);
    result.height = std::min(result.height, kScrollTrackHeight);

    const float travel = kScrollTrackHeight - result.height;
    result.y = kScrollTrackY + travel * (scrollPosition_ / maxScroll);
    result.visible = true;
    return result;
}

void TaskTracerHud::clampScroll() noexcept {
    const float total = scrollContentHeight();
    scrollPosition_ = std::clamp(
        scrollPosition_,
        0.0F,
        std::max(0.0F, total - kContentHeight));
}

eudoria::ui::Point TaskTracerHud::mappedRoot(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    return viewport.mapRoot(
        {
            kDefaultRoot.x + dragOffsetLegacy_.x,
            kDefaultRoot.y + dragOffsetLegacy_.y,
        },
        kAnchor);
}

TaskTracerHud::Rect TaskTracerHud::titleRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kTitleX * scale,
        root.y + kTitleY * scale,
        root.x + (kTitleX + kTitleWidth) * scale,
        root.y + (kTitleY + kTitleHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::collapseRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kCollapseTextX * scale,
        root.y + kCollapseTextY * scale,
        root.x + (kCollapseTextX + kCollapseTextWidth) * scale,
        root.y + (kCollapseTextY + kCollapseTextHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::haveTaskTabRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kHaveTaskTabX * scale,
        root.y + kTabY * scale,
        root.x + (kHaveTaskTabX + kTabHitWidth) * scale,
        root.y + (kTabY + kTabHitHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::canReceiveTaskTabRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kCanReceiveTaskTabX * scale,
        root.y + kTabY * scale,
        root.x + (kCanReceiveTaskTabX + kTabHitWidth) * scale,
        root.y + (kTabY + kTabHitHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::contentRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kPointTaskX * scale,
        root.y + kPointTaskY * scale,
        root.x + (kPointTaskX + kContentWidth) * scale,
        root.y + (kPointTaskY + kContentHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::scrollUpRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kScrollX * scale,
        root.y + kScrollY * scale,
        root.x + (kScrollX + kScrollWidth) * scale,
        root.y + (kScrollY + kScrollArrowHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::scrollDownRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    const float y = kScrollHeight - kScrollArrowHeight;
    return {
        root.x + kScrollX * scale,
        root.y + y * scale,
        root.x + (kScrollX + kScrollWidth) * scale,
        root.y + (y + kScrollArrowHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::scrollTrackRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kScrollX * scale,
        root.y + kScrollTrackY * scale,
        root.x + (kScrollX + kScrollWidth) * scale,
        root.y + (kScrollTrackY + kScrollTrackHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::scrollThumbRect(
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const auto geometry = thumbGeometry();
    if (!geometry.visible) {
        return {};
    }

    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    return {
        root.x + kScrollX * scale,
        root.y + geometry.y * scale,
        root.x + (kScrollX + kScrollWidth) * scale,
        root.y + (geometry.y + geometry.height) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::taskNameRect(
    const TaskVisual& task,
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    const float y = task.y - scrollPosition_ + kTaskNameY;
    return {
        root.x + kTaskNameX * scale,
        root.y + y * scale,
        root.x + (kTaskNameX + kTaskNameWidth) * scale,
        root.y + (y + kTaskNameHeight) * scale,
    };
}

TaskTracerHud::Rect TaskTracerHud::taskViewRect(
    const TaskVisual& task,
    const std::uint32_t width,
    const std::uint32_t height) const noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    const float scale = viewport.scale();
    const auto root = mappedRoot(width, height);
    const float y = task.y - scrollPosition_ + kTaskViewY;
    return {
        root.x + kTaskViewX * scale,
        root.y + y * scale,
        root.x + (kTaskViewX + kTaskViewWidth) * scale,
        root.y + (y + kTaskViewHeight) * scale,
    };
}

} // namespace eudoria::game::ui
