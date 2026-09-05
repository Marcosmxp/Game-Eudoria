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

bool ControlBar::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot,
    const std::filesystem::path& runtimeRoot) {
    referenceSkin_ = {};
    renderer.loadTexture((referenceRoot / L"control_bar.reference.png").wstring(), referenceSkin_);

    bool loadedAnyState = false;
    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto buttonRoot = runtimeRoot / std::string(specs[index].assetDirectory);
        auto& visual = visuals_[index];

        if (renderer.loadTexture((buttonRoot / L"over.png").wstring(), visual.over)) {
            loadedAnyState = true;
        }
        if (renderer.loadTexture((buttonRoot / L"down.png").wstring(), visual.down)) {
            loadedAnyState = true;
        }
    }

    return referenceSkin_.valid() || loadedAnyState;
}

void ControlBar::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    const HudWindowManager& windows) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    if (referenceSkin_.valid()) {
        renderer.draw(
            referenceSkin_,
            root.x - (kReferenceOriginX * scale),
            root.y - (kReferenceOriginY * scale),
            static_cast<float>(referenceSkin_.width) * scale,
            static_cast<float>(referenceSkin_.height) * scale);
    }

    const auto& specs = buttons();
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto& spec = specs[index];
        const auto& visual = visuals_[index];

        const SpriteTexture* state = nullptr;
        if (pressed_ == static_cast<int>(index) || windows.visible(spec.window)) {
            state = visual.down.valid() ? &visual.down : nullptr;
        } else if (hovered_ == static_cast<int>(index)) {
            state = visual.over.valid() ? &visual.over : nullptr;
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

    const auto& specs = buttons();
    if (hovered_ < 0 || static_cast<std::size_t>(hovered_) >= specs.size()) {
        return {};
    }
    return specs[static_cast<std::size_t>(hovered_)].id;
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
