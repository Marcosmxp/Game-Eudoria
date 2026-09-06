#include "game/ui/RoleWindowHud.h"

namespace eudoria::game::ui {
namespace {

TextTextureStyle labelStyle(
    const std::uint32_t size,
    const TextHorizontalAlign align = TextHorizontalAlign::Center) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = size;
    style.red = 255;
    style.green = 255;
    style.blue = 255;
    style.align = align;
    style.wordWrap = false;
    return style;
}

} // namespace

bool RoleWindowHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    bool loaded = true;

    loaded = renderer.loadTexture((runtimeRoot / L"background.png").wstring(), background_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"title_box.png").wstring(), titleBox_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"tab" / L"normal.png").wstring(), tabNormal_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"tab" / L"active.png").wstring(), tabActive_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"close" / L"up.png").wstring(), close_.up) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"close" / L"over.png").wstring(), close_.over) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"close" / L"down.png").wstring(), close_.down) && loaded;
    loaded = character_.initialize(renderer, runtimeRoot / L"character") && loaded;

    // PlayerBoxUIMC's title field is populated with the current player name by
    // the live controller. During the UI-only milestone PlayerInfo already uses
    // Eudoria as the neutral local player name, so the Role header mirrors it
    // instead of incorrectly hard-coding the word "Character".
    loaded = createTextTexture(
        renderer,
        L"Eudoria",
        static_cast<std::uint32_t>(kTitleTextWidth),
        static_cast<std::uint32_t>(kTitleTextHeight),
        labelStyle(12),
        titleLabel_) && loaded;

    loaded = createTextTexture(
        renderer,
        L"Character",
        static_cast<std::uint32_t>(kTabLabelWidth),
        static_cast<std::uint32_t>(kTabLabelHeight),
        labelStyle(10),
        characterLabel_) && loaded;

    loaded = createTextTexture(
        renderer,
        L"Divine Soul",
        static_cast<std::uint32_t>(kTabLabelWidth),
        static_cast<std::uint32_t>(kTabLabelHeight),
        labelStyle(10),
        divineSoulLabel_) && loaded;

    loaded = createTextTexture(
        renderer,
        L"Familiar",
        static_cast<std::uint32_t>(kTabLabelWidth),
        static_cast<std::uint32_t>(kTabLabelHeight),
        labelStyle(10),
        familiarLabel_) && loaded;

    rootX_ = kInitialRootX;
    rootY_ = kInitialRootY;
    selectedTab_ = Tab::Character;
    pressed_ = PressedAction::None;
    hovered_ = PressedAction::None;
    dragging_ = false;
    wasVisible_ = false;
    return loaded;
}

void RoleWindowHud::update(const HudWindowManager& windows) noexcept {
    const bool visible = windows.visible(HudWindow::Character);
    if (visible && !wasVisible_) {
        // PlayerBoxUI.showUI() closes every child panel and always re-opens the
        // Character tab first.
        selectedTab_ = Tab::Character;
        pressed_ = PressedAction::None;
        hovered_ = PressedAction::None;
        dragging_ = false;
    } else if (!visible && wasVisible_) {
        pressed_ = PressedAction::None;
        hovered_ = PressedAction::None;
        dragging_ = false;
    }

    if (!kFamiliarTabVisible && selectedTab_ == Tab::Familiar) {
        selectedTab_ = Tab::Character;
    }
    wasVisible_ = visible;
}

void RoleWindowHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    const HudWindowManager& windows) const {
    (void)viewportWidth;
    (void)viewportHeight;

    if (!windows.visible(HudWindow::Character)) {
        return;
    }

    // bgBox is depth 1 in PlayerBoxUIMC.
    if (background_.valid()) {
        renderer.draw(
            background_,
            rootX_ + kBackgroundPlacementX + (kBackgroundBoundsLeft * kBackgroundScaleX),
            rootY_ + kBackgroundPlacementY + (kBackgroundBoundsTop * kBackgroundScaleY),
            kBackgroundWidth * kBackgroundScaleX,
            kBackgroundHeight * kBackgroundScaleY);
    }

    // pointChildUI is depth 11. Render the selected child before the tabs/title,
    // exactly matching the PlayerBoxUIMC display order.
    if (selectedTab_ == Tab::Character) {
        character_.render(
            renderer,
            rootX_ + kChildPointX,
            rootY_ + kChildPointY);
    }

    // Tabs begin at depth 13, then titleBox/text and close button.
    drawTab(renderer, Tab::Character, characterLabel_);
    drawTab(renderer, Tab::DivineSoul, divineSoulLabel_);
    if (kFamiliarTabVisible) {
        drawTab(renderer, Tab::Familiar, familiarLabel_);
    }

    if (titleBox_.valid()) {
        renderer.draw(
            titleBox_,
            rootX_ + kTitlePlacementX + (kTitleBoundsLeft * kTitleScaleX),
            rootY_ + kTitlePlacementY + (kTitleBoundsTop * kTitleScaleY),
            kTitleWidth * kTitleScaleX,
            kTitleHeight * kTitleScaleY);
    }

    if (titleLabel_.texture.valid()) {
        renderer.draw(
            titleLabel_.texture,
            rootX_ + kTitleTextX,
            rootY_ + kTitleTextY,
            kTitleTextWidth,
            static_cast<float>(titleLabel_.texture.height));
    }

    const SpriteTexture* closeTexture = closeState();
    if (closeTexture && closeTexture->valid()) {
        renderer.draw(
            *closeTexture,
            rootX_ + kCloseX + kCloseRasterOffsetX,
            rootY_ + kCloseY + kCloseRasterOffsetY,
            static_cast<float>(closeTexture->width),
            static_cast<float>(closeTexture->height));
    }
}

bool RoleWindowHud::onMouseDown(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    const HudWindowManager& windows) noexcept {
    (void)viewportWidth;
    (void)viewportHeight;

    if (!windows.visible(HudWindow::Character)) {
        return false;
    }

    const float localX = mouseX - rootX_;
    const float localY = mouseY - rootY_;
    pressed_ = hitAction(localX, localY);
    hovered_ = pressed_;

    if (pressed_ == PressedAction::Drag) {
        dragging_ = true;
        dragOffsetX_ = mouseX - rootX_;
        dragOffsetY_ = mouseY - rootY_;
    }

    return pressed_ != PressedAction::None;
}

bool RoleWindowHud::onMouseMove(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    const HudWindowManager& windows) noexcept {
    (void)viewportWidth;
    (void)viewportHeight;

    if (!windows.visible(HudWindow::Character)) {
        return false;
    }

    if (dragging_) {
        rootX_ = mouseX - dragOffsetX_;
        rootY_ = mouseY - dragOffsetY_;
        hovered_ = PressedAction::Drag;
        return true;
    }

    hovered_ = hitAction(mouseX - rootX_, mouseY - rootY_);
    return hovered_ != PressedAction::None;
}

bool RoleWindowHud::onMouseUp(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight,
    HudWindowManager& windows) noexcept {
    (void)viewportWidth;
    (void)viewportHeight;

    if (!windows.visible(HudWindow::Character)) {
        pressed_ = PressedAction::None;
        dragging_ = false;
        return false;
    }

    if (dragging_) {
        dragging_ = false;
        pressed_ = PressedAction::None;
        hovered_ = hitAction(mouseX - rootX_, mouseY - rootY_);
        return true;
    }

    const PressedAction released = hitAction(mouseX - rootX_, mouseY - rootY_);
    const PressedAction pressed = pressed_;
    pressed_ = PressedAction::None;
    hovered_ = released;

    if (pressed == PressedAction::None || pressed != released) {
        return false;
    }

    if (pressed == PressedAction::Close) {
        windows.hide(HudWindow::Character);
        hovered_ = PressedAction::None;
        return true;
    }

    if (pressed == PressedAction::CharacterTab ||
        pressed == PressedAction::DivineSoulTab ||
        (kFamiliarTabVisible && pressed == PressedAction::FamiliarTab)) {
        selectedTab_ = tabForAction(pressed);
        return true;
    }

    return pressed == PressedAction::Drag;
}

RoleWindowHud::Rect RoleWindowHud::backgroundRect() const noexcept {
    return {
        kBackgroundPlacementX + (kBackgroundBoundsLeft * kBackgroundScaleX),
        kBackgroundPlacementY + (kBackgroundBoundsTop * kBackgroundScaleY),
        kBackgroundPlacementX + ((kBackgroundBoundsLeft + kBackgroundWidth) * kBackgroundScaleX),
        kBackgroundPlacementY + ((kBackgroundBoundsTop + kBackgroundHeight) * kBackgroundScaleY),
    };
}

RoleWindowHud::Rect RoleWindowHud::closeRect() const noexcept {
    return {
        kCloseX + kCloseHitLeft,
        kCloseY + kCloseHitTop,
        kCloseX + kCloseHitLeft + kCloseHitSize,
        kCloseY + kCloseHitTop + kCloseHitSize,
    };
}

RoleWindowHud::Rect RoleWindowHud::tabRect(const Tab tab) const noexcept {
    const float x = tabX(tab);
    return {
        x + (kTabActiveBoundsX * kTabScaleX),
        kTabY + (kTabActiveBoundsY * kTabScaleY),
        x + ((kTabActiveBoundsX + kTabActiveWidth) * kTabScaleX),
        kTabY + ((kTabActiveBoundsY + kTabActiveHeight) * kTabScaleY),
    };
}

float RoleWindowHud::tabX(const Tab tab) noexcept {
    switch (tab) {
    case Tab::Character: return kCharacterTabX;
    case Tab::DivineSoul: return kDivineSoulTabX;
    case Tab::Familiar: return kFamiliarTabX;
    }
    return kCharacterTabX;
}

RoleWindowHud::PressedAction RoleWindowHud::hitAction(
    const float localX,
    const float localY) const noexcept {
    if (closeRect().contains(localX, localY)) {
        return PressedAction::Close;
    }
    if (tabRect(Tab::Character).contains(localX, localY)) {
        return PressedAction::CharacterTab;
    }
    if (tabRect(Tab::DivineSoul).contains(localX, localY)) {
        return PressedAction::DivineSoulTab;
    }
    if (kFamiliarTabVisible && tabRect(Tab::Familiar).contains(localX, localY)) {
        return PressedAction::FamiliarTab;
    }
    if (backgroundRect().contains(localX, localY)) {
        return PressedAction::Drag;
    }
    return PressedAction::None;
}

RoleWindowHud::Tab RoleWindowHud::tabForAction(const PressedAction action) noexcept {
    switch (action) {
    case PressedAction::DivineSoulTab: return Tab::DivineSoul;
    case PressedAction::FamiliarTab: return Tab::Familiar;
    case PressedAction::CharacterTab:
    default:
        return Tab::Character;
    }
}

const SpriteTexture* RoleWindowHud::closeState() const noexcept {
    if (pressed_ == PressedAction::Close && close_.down.valid()) {
        return &close_.down;
    }
    if (hovered_ == PressedAction::Close && close_.over.valid()) {
        return &close_.over;
    }
    if (close_.up.valid()) {
        return &close_.up;
    }
    return nullptr;
}

void RoleWindowHud::drawTab(
    SpriteRenderer& renderer,
    const Tab tab,
    const TextTextureResult& label) const {
    const bool active = selectedTab_ == tab;
    const SpriteTexture& texture = active ? tabActive_ : tabNormal_;
    const float boundsX = active ? kTabActiveBoundsX : kTabNormalBoundsX;
    const float boundsY = active ? kTabActiveBoundsY : kTabNormalBoundsY;
    const float x = tabX(tab);

    if (texture.valid()) {
        renderer.draw(
            texture,
            rootX_ + x + (boundsX * kTabScaleX),
            rootY_ + kTabY + (boundsY * kTabScaleY),
            static_cast<float>(texture.width) * kTabScaleX,
            static_cast<float>(texture.height) * kTabScaleY);
    }

    if (label.texture.valid()) {
        const Rect hit = tabRect(tab);
        const float labelX = (hit.left + hit.right - kTabLabelWidth) * 0.5F;
        renderer.draw(
            label.texture,
            rootX_ + labelX,
            rootY_ + kTabY + 1.0F,
            kTabLabelWidth,
            static_cast<float>(label.texture.height));
    }
}

} // namespace eudoria::game::ui
