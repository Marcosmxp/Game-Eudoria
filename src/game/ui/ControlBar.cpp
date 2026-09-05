#include "game/ui/ControlBar.h"

#include <algorithm>
#include <string>

namespace eudoria::game::ui {

const std::array<ControlBar::ButtonSpec, 13>& ControlBar::buttons() noexcept {
    static constexpr std::array<ButtonSpec, 13> value{{
        {"cmdRole", "cmdRole", {-425.05F, -50.00F}, {-39.95F, -29.00F}, {-442.0F, -77.0F, -409.0F, -44.0F}, HudWindow::Character, 'C'},
        {"cmdPet", "cmdPet", {-377.40F, -48.00F}, {-42.60F, -27.00F}, {-397.0F, -73.0F, -357.0F, -44.0F}, HudWindow::Pet, 'X'},
        {"cmdRide", "cmdRide", {-329.75F, -48.25F}, {-43.25F, -33.75F}, {-350.0F, -80.0F, -310.0F, -40.0F}, HudWindow::Mount, 'N'},
        {"cmdWing", "cmdWing", {-282.10F, -48.00F}, {-43.90F, -31.00F}, {-302.0F, -77.0F, -263.0F, -42.0F}, HudWindow::Wing, 'J'},
        {"cmdBag", "cmdBag", {-234.45F, -45.00F}, {-45.55F, -28.00F}, {-249.0F, -71.0F, -220.0F, -42.0F}, HudWindow::Inventory, 'B'},
        {"cmdSkill", "cmdSkill", {-186.60F, -47.65F}, {-46.40F, -29.35F}, {-204.0F, -75.0F, -171.0F, -41.0F}, HudWindow::Skills, 'V'},
        {"cmdBot", "cmdBot", {184.95F, -48.40F}, {-42.95F, -26.60F}, {167.0F, -73.0F, 203.0F, -42.0F}, HudWindow::Bot, 0},
        {"cmdQuest", "cmdQuest", {232.90F, -48.00F}, {-42.90F, -29.00F}, {216.0F, -75.0F, 249.0F, -43.0F}, HudWindow::Quests, 'T'},
        {"cmdFriend", "cmdFriend", {280.85F, -46.15F}, {-43.85F, -29.85F}, {265.0F, -74.0F, 296.0F, -43.0F}, HudWindow::Friends, 'O'},
        {"cmdTeam", "cmdTeam", {328.80F, -44.00F}, {-44.80F, -37.00F}, {310.0F, -79.0F, 346.0F, -44.0F}, HudWindow::Team, 'P'},
        {"cmdFamily", "cmdFamily", {376.75F, -48.00F}, {-43.75F, -29.00F}, {360.0F, -75.0F, 393.0F, -42.0F}, HudWindow::Guild, 'G'},
        {"cmdBlessGod", "cmdBlessGod", {424.80F, -44.00F}, {-42.80F, -33.00F}, {409.0F, -75.0F, 441.0F, -43.0F}, HudWindow::Count, 0},
        {"cmdSys", "cmdSys", {-458.60F, -30.55F}, {-43.40F, -12.45F}, {-467.0F, -41.0F, -450.0F, -28.0F}, HudWindow::System, 0},
    }};
    return value;
}

const ControlBar::Rect& ControlBar::soundHitRect() noexcept {
    static constexpr Rect value{-468.0F, -24.0F, -448.0F, -5.0F};
    return value;
}

const ControlBar::Rect& ControlBar::totalMenuToggleHitRect() noexcept {
    static constexpr Rect value{452.0F, -39.0F, 470.0F, -20.0F};
    return value;
}

bool ControlBar::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    baseSkin_ = {};
    totalMenuSkin_ = {};
    soundOn_ = {};
    soundOff_ = {};

    bool loaded = renderer.loadTexture((runtimeRoot / L"base.png").wstring(), baseSkin_);
    loaded = renderer.loadTexture((runtimeRoot / L"total_icon.png").wstring(), totalMenuSkin_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"sound" / L"on.png").wstring(), soundOn_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"sound" / L"off.png").wstring(), soundOff_) && loaded;

    const auto loadButtonVisual =
        [&renderer](const std::filesystem::path& root, ButtonVisual& visual) {
            const bool up = renderer.loadTexture((root / L"up.png").wstring(), visual.up);
            const bool over = renderer.loadTexture((root / L"over.png").wstring(), visual.over);
            const bool down = renderer.loadTexture((root / L"down.png").wstring(), visual.down);
            return up && over && down;
        };

    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto buttonRoot = runtimeRoot / std::string(specs[index].assetDirectory);
        loaded = loadButtonVisual(buttonRoot, visuals_[index]) && loaded;
    }

    loaded = loadButtonVisual(runtimeRoot / L"up", expandVisual_) && loaded;
    loaded = loadButtonVisual(runtimeRoot / L"down", collapseVisual_) && loaded;

    totalMenuExpanded_ = true;
    totalMenuAnimating_ = false;
    totalMenuTweenStartY_ = kTotalMenuInitialY;
    totalMenuTweenTargetY_ = kTotalMenuInitialY;
    return loaded;
}

void ControlBar::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    const HudWindowManager& windows) const {
    (void)windows;
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    if (baseSkin_.valid()) {
        renderer.draw(
            baseSkin_,
            root.x + (kBaseImageOffset.x * scale),
            root.y + (kBaseImageOffset.y * scale),
            static_cast<float>(baseSkin_.width) * scale,
            static_cast<float>(baseSkin_.height) * scale);
    }

    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto& spec = specs[index];
        const auto& visual = visuals_[index];

        const SpriteTexture* state = visual.up.valid() ? &visual.up : nullptr;
        if (pressed_ == static_cast<int>(index) && visual.down.valid()) {
            state = &visual.down;
        } else if (hovered_ == static_cast<int>(index) && visual.over.valid()) {
            state = &visual.over;
        }

        if (!state) {
            continue;
        }

        const float x = root.x + ((spec.placement.x + spec.imageOffset.x) * scale);
        const float y = root.y + ((spec.placement.y + spec.imageOffset.y) * scale);
        renderer.draw(
            *state,
            x,
            y,
            static_cast<float>(state->width) * scale,
            static_cast<float>(state->height) * scale);
    }

    const SpriteTexture& sound = soundEnabled_ ? soundOn_ : soundOff_;
    if (sound.valid()) {
        renderer.draw(
            sound,
            root.x + ((kSoundPlacement.x + kSoundImageOffset.x) * scale),
            root.y + ((kSoundPlacement.y + kSoundImageOffset.y) * scale),
            static_cast<float>(sound.width) * scale,
            static_cast<float>(sound.height) * scale);
    }

    const auto& toggleVisual = totalMenuExpanded_ ? collapseVisual_ : expandVisual_;
    const SpriteTexture* toggleState =
        toggleVisual.up.valid() ? &toggleVisual.up : nullptr;
    if (pressed_ == -3 && toggleVisual.down.valid()) {
        toggleState = &toggleVisual.down;
    } else if (hovered_ == -3 && toggleVisual.over.valid()) {
        toggleState = &toggleVisual.over;
    }

    if (toggleState) {
        renderer.draw(
            *toggleState,
            root.x + ((kTotalMenuTogglePlacement.x + kTotalMenuToggleImageOffset.x) * scale),
            root.y + ((kTotalMenuTogglePlacement.y + kTotalMenuToggleImageOffset.y) * scale),
            static_cast<float>(toggleState->width) * scale,
            static_cast<float>(toggleState->height) * scale);
    }

    renderTotalMenu(renderer, root, scale);
}

void ControlBar::renderTotalMenu(
    SpriteRenderer& renderer,
    const eudoria::ui::Point& root,
    const float scale) const {
    if (!totalMenuShouldRender() || !totalMenuSkin_.valid()) {
        return;
    }

    const float textureWidth = static_cast<float>(totalMenuSkin_.width);
    const float textureHeight = static_cast<float>(totalMenuSkin_.height);

    const float imageLeft = kTotalMenuX + kTotalMenuRasterMinX;
    const float imageTop = currentTotalMenuY() + kTotalMenuRasterMinY;

    const float sourceLeft = std::clamp(
        kTotalMenuClip.left - imageLeft,
        0.0F,
        textureWidth);
    const float sourceRight = std::clamp(
        kTotalMenuClip.right - imageLeft,
        0.0F,
        textureWidth);
    const float sourceTop = std::max(
        kTotalMenuConfigCropTop,
        std::clamp(kTotalMenuClip.top - imageTop, 0.0F, textureHeight));
    const float sourceBottom = std::clamp(
        kTotalMenuClip.bottom - imageTop,
        0.0F,
        textureHeight);

    if (sourceRight <= sourceLeft || sourceBottom <= sourceTop) {
        return;
    }

    const float visibleWidth = sourceRight - sourceLeft;
    const float visibleHeight = sourceBottom - sourceTop;
    const float legacyX = imageLeft + sourceLeft;
    const float legacyY = imageTop + sourceTop;

    renderer.drawRegion(
        totalMenuSkin_,
        SpriteSourceRect{sourceLeft, sourceTop, visibleWidth, visibleHeight},
        root.x + (legacyX * scale),
        root.y + (legacyY * scale),
        visibleWidth * scale,
        visibleHeight * scale);
}

void ControlBar::onMouseMove(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    const auto local = toLocal(mouseX, mouseY, viewportWidth, viewportHeight);
    hovered_ = hitTest(local);
    if (hovered_ < 0 && soundHitRect().contains(local.x, local.y)) {
        hovered_ = -2;
    } else if (hovered_ < 0 && totalMenuToggleHitRect().contains(local.x, local.y)) {
        hovered_ = -3;
    }
}

void ControlBar::onMouseDown(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    const auto local = toLocal(mouseX, mouseY, viewportWidth, viewportHeight);
    pressed_ = hitTest(local);
    if (pressed_ < 0 && soundHitRect().contains(local.x, local.y)) {
        pressed_ = -2;
    } else if (pressed_ < 0 && totalMenuToggleHitRect().contains(local.x, local.y)) {
        pressed_ = -3;
    }
}

bool ControlBar::onMouseUp(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    HudWindowManager& windows) noexcept {
    const auto local = toLocal(mouseX, mouseY, viewportWidth, viewportHeight);
    const int released = hitTest(local);
    bool handled = false;

    if (pressed_ >= 0 && pressed_ == released) {
        handled = activate(static_cast<std::size_t>(pressed_), windows);
    } else if (pressed_ == -2 && soundHitRect().contains(local.x, local.y)) {
        soundEnabled_ = !soundEnabled_;
        handled = true;
    } else if (pressed_ == -3 && totalMenuToggleHitRect().contains(local.x, local.y)) {
        toggleTotalMenu();
        handled = true;
    }

    pressed_ = -1;
    onMouseMove(mouseX, mouseY, viewportWidth, viewportHeight);
    return handled;
}

bool ControlBar::onKeyUp(const std::uint32_t virtualKey, HudWindowManager& windows) noexcept {
    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        if (specs[index].shortcut != 0 && specs[index].shortcut == virtualKey) {
            return activate(index, windows);
        }
    }
    return false;
}

std::string_view ControlBar::hoveredId() const noexcept {
    if (hovered_ == -2) {
        return "cmdSoundSwitch";
    }
    if (hovered_ == -3) {
        return totalMenuExpanded_ ? "down" : "up";
    }

    const auto& specs = buttons();
    if (hovered_ < 0 || static_cast<std::size_t>(hovered_) >= specs.size()) {
        return {};
    }
    return specs[static_cast<std::size_t>(hovered_)].id;
}

void ControlBar::toggleTotalMenu() noexcept {
    totalMenuTweenStartY_ = currentTotalMenuY();
    totalMenuTweenStarted_ = std::chrono::steady_clock::now();
    totalMenuAnimating_ = true;

    if (totalMenuExpanded_) {
        totalMenuExpanded_ = false;
        totalMenuTweenTargetY_ = kTotalMenuCollapsedY;
    } else {
        totalMenuExpanded_ = true;
        totalMenuTweenTargetY_ = kTotalMenuExpandedY;
    }
}

float ControlBar::currentTotalMenuY() const noexcept {
    if (!totalMenuAnimating_) {
        return totalMenuTweenTargetY_;
    }

    const auto elapsed = std::chrono::steady_clock::now() - totalMenuTweenStarted_;
    const float durationSeconds =
        std::chrono::duration<float>(kTotalMenuTweenDuration).count();
    const float elapsedSeconds = std::chrono::duration<float>(elapsed).count();
    const float linear = std::clamp(
        durationSeconds > 0.0F ? elapsedSeconds / durationSeconds : 1.0F,
        0.0F,
        1.0F);

    const float eased = 1.0F - ((1.0F - linear) * (1.0F - linear));
    return totalMenuTweenStartY_ +
        ((totalMenuTweenTargetY_ - totalMenuTweenStartY_) * eased);
}

bool ControlBar::totalMenuShouldRender() const noexcept {
    if (totalMenuExpanded_) {
        return true;
    }
    if (!totalMenuAnimating_) {
        return false;
    }
    return std::chrono::steady_clock::now() - totalMenuTweenStarted_ <
        kTotalMenuTweenDuration;
}

eudoria::ui::Point ControlBar::toLocal(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = std::max(viewport.scale(), 0.0001F);
    const auto root = viewport.mapRoot(kRoot, kAnchor);
    return {
        (mouseX - root.x) / scale,
        (mouseY - root.y) / scale,
    };
}

int ControlBar::hitTest(const eudoria::ui::Point local) noexcept {
    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        if (specs[index].hitRect.contains(local.x, local.y)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool ControlBar::activate(const std::size_t index, HudWindowManager& windows) noexcept {
    const auto& specs = buttons();
    if (index >= specs.size()) {
        return false;
    }

    const auto target = specs[index].window;
    if (target == HudWindow::Count) {
        return false;
    }

    windows.toggle(target);
    return true;
}

} // namespace eudoria::game::ui
