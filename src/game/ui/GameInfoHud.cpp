#include "game/ui/GameInfoHud.h"

#include <Windows.h>

#include <algorithm>
#include <cstddef>
#include <cwctype>
#include <utility>

namespace eudoria::game::ui {
namespace {

constexpr std::array<float, 7> kSendCenters{25.0F, 75.0F, 125.0F, 175.0F, 225.0F, 274.95F, 324.55F};
constexpr std::array<float, 6> kDisplayCenters{24.0F, 74.0F, 123.5F, 173.5F, 223.5F, 273.45F};

constexpr float kEnterCenterX = 290.35F;
constexpr float kEnterCenterY = -4.65F;
constexpr float kFaceCenterX = 305.35F;
constexpr float kFaceCenterY = -12.15F;

std::wstring scopePrefix(const ChatScope scope) {
    switch (scope) {
    case ChatScope::Nearby: return L"[Local] ";
    case ChatScope::Whisper: return L"[Whisper] ";
    case ChatScope::Team: return L"[Team] ";
    case ChatScope::Family: return L"[Guild] ";
    case ChatScope::World: return L"[World] ";
    case ChatScope::Trade: return L"[Trade] ";
    case ChatScope::System: return L"[System] ";
    case ChatScope::All: break;
    }
    return {};
}

bool isPrintable(const wchar_t value) {
    return value >= 0x20 && value != 0x7F;
}

} // namespace

bool GameInfoHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot,
    const std::filesystem::path& runtimeRoot) {
    renderer_ = &renderer;

    const bool loadedSkin = renderer.loadTexture((referenceRoot / L"game_info.reference.png").wstring(), skin_);
    renderer.loadTexture((runtimeRoot / L"scope_button/normal.png").wstring(), scopeNormal_);
    renderer.loadTexture((runtimeRoot / L"scope_button/active.png").wstring(), scopeActive_);

    auto loadButton = [&renderer, &runtimeRoot](const wchar_t* directory, ButtonStates& states) {
        renderer.loadTexture((runtimeRoot / directory / L"up.png").wstring(), states.up);
        renderer.loadTexture((runtimeRoot / directory / L"over.png").wstring(), states.over);
        renderer.loadTexture((runtimeRoot / directory / L"down.png").wstring(), states.down);
    };
    loadButton(L"enter", enterButton_);
    loadButton(L"face", faceButton_);
    loadButton(L"content_toggle", hideButton_);

    sendScopes_ = {{
        {ChatScope::Nearby, kSendCenters[0], L"Local", false, {}, {}},
        {ChatScope::Whisper, kSendCenters[1], L"Whisper", false, {}, {}},
        {ChatScope::Team, kSendCenters[2], L"Team", false, {}, {}},
        {ChatScope::Family, kSendCenters[3], L"Guild", false, {}, {}},
        {ChatScope::World, kSendCenters[4], L"World", false, {}, {}},
        {ChatScope::Trade, kSendCenters[5], L"Trade", false, {}, {}},
        {ChatScope::World, kSendCenters[6], L"Speaker", false, {}, {}},
    }};

    displayScopes_ = {{
        {ChatScope::All, kDisplayCenters[0], L"All", true, {}, {}},
        {ChatScope::Whisper, kDisplayCenters[1], L"Whisper", true, {}, {}},
        {ChatScope::Team, kDisplayCenters[2], L"Team", true, {}, {}},
        {ChatScope::Family, kDisplayCenters[3], L"Guild", true, {}, {}},
        {ChatScope::World, kDisplayCenters[4], L"World", true, {}, {}},
        {ChatScope::Trade, kDisplayCenters[5], L"Trade", true, {}, {}},
    }};

    rebuildScopeLabels();

    for (std::size_t index = 0; index < faceLabels_.size(); ++index) {
        wchar_t buffer[4]{};
        swprintf_s(buffer, 4, L"%02u", static_cast<unsigned>(index));
        TextTextureStyle style;
        style.fontFamily = L"Arial";
        style.fontPixelHeight = 9;
        style.red = 235;
        style.green = 235;
        style.blue = 220;
        style.align = TextHorizontalAlign::Center;
        style.wordWrap = false;
        createTextTexture(renderer, buffer, 22, 17, style, faceLabels_[index]);
    }

    // Development-only local messages exercise the reconstructed message pipeline.
    // They are not Crystal Saga content and disappear once the offline game-state feed is connected.
    receiveMessage({ChatScope::System, L"", L"Eudoria offline client initialized."});
    receiveMessage({ChatScope::System, L"", L"GameInfoUIMC runtime loaded from the legacy payload."});

    rebuildInputTexture();
    rebuildMessages();
    return loadedSkin;
}

void GameInfoHud::update() {
    if (dirtyInput_) rebuildInputTexture();
    if (dirtyMessages_) rebuildMessages();
}

void GameInfoHud::receiveMessage(ChatMessage message) {
    messages_.push_back(std::move(message));
    trimMessages();
    dirtyMessages_ = true;
}

void GameInfoHud::trimMessages() {
    if (messages_.size() > kMessageLimit) {
        messages_.erase(messages_.begin(), messages_.begin() + static_cast<std::ptrdiff_t>(messages_.size() - kMessageLimit));
    }
}

TextTextureStyle GameInfoHud::scopeTextStyle(const bool active) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = 9;
    style.red = active ? 255 : 235;
    style.green = active ? 210 : 235;
    style.blue = active ? 85 : 235;
    style.align = TextHorizontalAlign::Center;
    style.wordWrap = false;
    return style;
}

TextTextureStyle GameInfoHud::messageTextStyle(const ChatScope scope) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = 10;
    style.wordWrap = true;

    switch (scope) {
    case ChatScope::Whisper:
        style.red = 255; style.green = 150; style.blue = 235;
        break;
    case ChatScope::Team:
        style.red = 130; style.green = 235; style.blue = 255;
        break;
    case ChatScope::Family:
        style.red = 255; style.green = 215; style.blue = 90;
        break;
    case ChatScope::World:
        style.red = 255; style.green = 170; style.blue = 75;
        break;
    case ChatScope::Trade:
        style.red = 150; style.green = 255; style.blue = 145;
        break;
    case ChatScope::System:
        style.red = 255; style.green = 90; style.blue = 70;
        break;
    case ChatScope::Nearby:
    case ChatScope::All:
        style.red = 245; style.green = 245; style.blue = 245;
        break;
    }
    return style;
}

void GameInfoHud::rebuildScopeLabels() {
    if (!renderer_) return;
    for (auto& button : sendScopes_) {
        createTextTexture(*renderer_, button.label, 48, 17, scopeTextStyle(false), button.normalLabel);
        createTextTexture(*renderer_, button.label, 48, 17, scopeTextStyle(true), button.activeLabel);
    }
    for (auto& button : displayScopes_) {
        createTextTexture(*renderer_, button.label, 48, 17, scopeTextStyle(false), button.normalLabel);
        createTextTexture(*renderer_, button.label, 48, 17, scopeTextStyle(true), button.activeLabel);
    }
}

void GameInfoHud::rebuildInputTexture() {
    inputTexture_ = {};
    dirtyInput_ = false;
    if (!renderer_) return;

    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = 11;
    style.red = 25;
    style.green = 25;
    style.blue = 25;
    style.wordWrap = false;

    std::wstring shown = input_;
    if (inputFocused_) shown += L"|";
    createTextTexture(*renderer_, shown, static_cast<std::uint32_t>(kInputWidth - 8.0F), 18, style, inputTexture_);
}

bool GameInfoHud::messageVisible(const ChatScope scope) const noexcept {
    if (displayScope_ == ChatScope::All) return true;
    if (scope == ChatScope::System) return true;
    return scope == displayScope_;
}

void GameInfoHud::rebuildMessages() {
    messageVisuals_.clear();
    totalMessageHeight_ = 0.0F;
    dirtyMessages_ = false;
    if (!renderer_) return;

    for (const auto& message : messages_) {
        if (!messageVisible(message.scope)) continue;

        std::wstring text = scopePrefix(message.scope);
        if (!message.sender.empty()) {
            text += L"[" + message.sender + L"]: ";
        }
        text += message.text;

        MessageVisual visual;
        visual.scope = message.scope;
        visual.y = totalMessageHeight_;
        createTextTexture(
            *renderer_,
            text,
            static_cast<std::uint32_t>(kMessageWidth),
            256,
            messageTextStyle(message.scope),
            visual.text);
        visual.height = static_cast<float>(std::max<std::uint32_t>(visual.text.contentHeight, 13));
        totalMessageHeight_ += visual.height;
        messageVisuals_.push_back(std::move(visual));
    }

    messageScroll_ = std::max(0.0F, totalMessageHeight_ - infoHeight_);
    clampMessageScroll();
}

void GameInfoHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const float contentY = -(infoHeight_ + kContentBaseOffset);

    if (skin_.valid()) {
        renderer.drawRegion(
            skin_,
            SpriteSourceRect{0.0F, 315.0F, 362.0F, 34.0F},
            root.x - kReferenceOriginX * scale,
            root.y - 33.5F * scale,
            362.0F * scale,
            34.0F * scale);
    }

    for (std::size_t index = 0; index < sendScopes_.size(); ++index) {
        const auto& button = sendScopes_[index];
        const bool active = index < 6 && button.scope == sendScope_;
        const auto& background = active && scopeActive_.valid() ? scopeActive_ : scopeNormal_;
        const float left = button.centerX - (kScopeButtonWidth * 0.5F);
        const float top = kSendButtonY - (kScopeButtonHeight * 0.5F);
        if (background.valid()) {
            renderer.draw(background, root.x + left * scale, root.y + top * scale,
                kScopeButtonWidth * scale, kScopeButtonHeight * scale);
        }
        const auto& label = active ? button.activeLabel : button.normalLabel;
        if (label.texture.valid()) {
            renderer.draw(label.texture, root.x + left * scale, root.y + (top + 2.0F) * scale,
                48.0F * scale, 17.0F * scale);
        }
    }

    if (inputTexture_.texture.valid()) {
        renderer.draw(inputTexture_.texture,
            root.x + (kInputX + 4.0F) * scale,
            root.y + (kInputY + 2.0F) * scale,
            static_cast<float>(inputTexture_.texture.width) * scale,
            static_cast<float>(inputTexture_.texture.height) * scale);
    }

    auto drawButtonState = [&renderer, scale, &root](const ButtonStates& states, const bool hovered, const bool pressed,
                                                      const float centerX, const float centerY,
                                                      const float width, const float height) {
        const SpriteTexture* texture = &states.up;
        if (pressed && states.down.valid()) texture = &states.down;
        else if (hovered && states.over.valid()) texture = &states.over;
        if (!texture->valid()) return;
        renderer.draw(*texture,
            root.x + (centerX - width * 0.5F) * scale,
            root.y + (centerY - height * 0.5F) * scale,
            width * scale,
            height * scale);
    };

    const bool enterPressed = pressedAction_ == PressedAction::Enter;
    const bool facePressed = pressedAction_ == PressedAction::Face;
    drawButtonState(enterButton_, hoveredEnter_, enterPressed, kEnterCenterX, kEnterCenterY, 19.0F, 18.0F);
    drawButtonState(faceButton_, hoveredFace_, facePressed, kFaceCenterX, kFaceCenterY, 21.0F, 21.0F);

    const float clipTop = contentY + kMessageLocalY;
    const float clipBottom = clipTop + infoHeight_;
    for (const auto& visual : messageVisuals_) {
        if (!visual.text.texture.valid()) continue;
        drawClipped(
            renderer,
            visual.text.texture,
            contentOffsetX_ + kMessageX,
            clipTop + visual.y - messageScroll_,
            static_cast<float>(visual.text.texture.width),
            static_cast<float>(visual.text.texture.height),
            clipTop,
            clipBottom,
            root,
            scale);
    }

    if (contentHovered_ || contentOffsetX_ < 0.0F) {
        for (std::size_t index = 0; index < displayScopes_.size(); ++index) {
            const auto& button = displayScopes_[index];
            const bool active = button.scope == displayScope_;
            const auto& background = active && scopeActive_.valid() ? scopeActive_ : scopeNormal_;
            const float centerY = contentY + kReceiveButtonLocalY;
            const float left = contentOffsetX_ + button.centerX - (kScopeButtonWidth * 0.5F);
            const float top = centerY - (kScopeButtonHeight * 0.5F);
            if (background.valid()) {
                renderer.draw(background, root.x + left * scale, root.y + top * scale,
                    kScopeButtonWidth * scale, kScopeButtonHeight * scale);
            }
            const auto& label = active ? button.activeLabel : button.normalLabel;
            if (label.texture.valid()) {
                renderer.draw(label.texture, root.x + left * scale, root.y + (top + 2.0F) * scale,
                    48.0F * scale, 17.0F * scale);
            }
        }

        if (skin_.valid() && contentOffsetX_ >= 0.0F) {
            renderer.drawRegion(
                skin_,
                SpriteSourceRect{10.0F, 108.0F, 18.0F, 185.0F},
                root.x + contentOffsetX_ * scale,
                root.y + clipTop * scale,
                18.0F * scale,
                infoHeight_ * scale);

            renderer.drawRegion(
                skin_,
                SpriteSourceRect{325.0F, 85.0F, 36.0F, 22.0F},
                root.x + (contentOffsetX_ + 315.0F) * scale,
                root.y + (contentY - 3.5F) * scale,
                36.0F * scale,
                22.0F * scale);
        } else if (contentOffsetX_ < 0.0F && hideButton_.up.valid()) {
            renderer.draw(hideButton_.up,
                root.x - 8.5F * scale,
                root.y + (contentY - 2.5F) * scale,
                17.0F * scale,
                17.0F * scale);
        }
    }

    if (faceOpen_ && skin_.valid()) {
        constexpr float faceX = 338.15F;
        constexpr float faceY = -190.0F;
        constexpr float faceWidth = 215.0F;
        constexpr float faceHeight = 190.0F;
        renderer.drawRegion(
            skin_,
            SpriteSourceRect{348.0F, 341.0F, 320.0F, 168.0F},
            root.x + faceX * scale,
            root.y + faceY * scale,
            faceWidth * scale,
            faceHeight * scale);

        for (std::size_t index = 0; index < faceLabels_.size(); ++index) {
            const auto& label = faceLabels_[index];
            if (!label.texture.valid()) continue;
            const float x = faceX + 10.0F + static_cast<float>(index % 8) * 25.0F;
            const float y = faceY + 10.0F + static_cast<float>(index / 8) * 25.0F;
            renderer.draw(label.texture, root.x + x * scale, root.y + y * scale, 22.0F * scale, 17.0F * scale);
        }
    }
}

eudoria::ui::Point GameInfoHud::mappedRoot(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    return viewport.mapRoot(kRoot, kAnchor);
}

eudoria::ui::Point GameInfoHud::screenToLegacy(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = std::max(viewport.scale(), 0.0001F);
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    return {(mouseX - root.x) / scale, (mouseY - root.y) / scale};
}

GameInfoHud::Rect GameInfoHud::inputRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    return {root.x + kInputX * scale, root.y + kInputY * scale,
            root.x + (kInputX + kInputWidth) * scale, root.y + (kInputY + kInputHeight) * scale};
}

GameInfoHud::Rect GameInfoHud::contentRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const float contentY = -(infoHeight_ + kContentBaseOffset);
    const float leftLegacy = contentOffsetX_;
    const float rightLegacy = contentOffsetX_ + 351.0F;
    return {root.x + leftLegacy * scale, root.y + contentY * scale,
            root.x + rightLegacy * scale, root.y + (contentY + infoHeight_ + 52.0F) * scale};
}

GameInfoHud::Rect GameInfoHud::sendButtonRect(
    const std::size_t index,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const float left = sendScopes_[index].centerX - kScopeButtonWidth * 0.5F;
    const float top = kSendButtonY - kScopeButtonHeight * 0.5F;
    return {root.x + left * scale, root.y + top * scale,
            root.x + (left + kScopeButtonWidth) * scale, root.y + (top + kScopeButtonHeight) * scale};
}

GameInfoHud::Rect GameInfoHud::displayButtonRect(
    const std::size_t index,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const float contentY = -(infoHeight_ + kContentBaseOffset);
    const float centerY = contentY + kReceiveButtonLocalY;
    const float left = contentOffsetX_ + displayScopes_[index].centerX - kScopeButtonWidth * 0.5F;
    const float top = centerY - kScopeButtonHeight * 0.5F;
    return {root.x + left * scale, root.y + top * scale,
            root.x + (left + kScopeButtonWidth) * scale, root.y + (top + kScopeButtonHeight) * scale};
}

GameInfoHud::Rect GameInfoHud::enterRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    return {root.x + (kEnterCenterX - 9.5F) * scale, root.y + (kEnterCenterY - 9.0F) * scale,
            root.x + (kEnterCenterX + 9.5F) * scale, root.y + (kEnterCenterY + 9.0F) * scale};
}

GameInfoHud::Rect GameInfoHud::faceRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    return {root.x + (kFaceCenterX - 10.5F) * scale, root.y + (kFaceCenterY - 10.5F) * scale,
            root.x + (kFaceCenterX + 10.5F) * scale, root.y + (kFaceCenterY + 10.5F) * scale};
}

GameInfoHud::Rect GameInfoHud::hideRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const float contentY = -(infoHeight_ + kContentBaseOffset);
    const float cx = contentOffsetX_ + 333.95F;
    const float cy = contentY + 6.0F;
    return {root.x + (cx - 9.0F) * scale, root.y + (cy - 9.0F) * scale,
            root.x + (cx + 9.0F) * scale, root.y + (cy + 9.0F) * scale};
}

GameInfoHud::Rect GameInfoHud::resizeRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    const float contentY = -(infoHeight_ + kContentBaseOffset);
    const float cx = contentOffsetX_ + 317.95F;
    const float cy = contentY + 22.0F;
    return {root.x + (cx - 9.0F) * scale, root.y + (cy - 9.0F) * scale,
            root.x + (cx + 9.0F) * scale, root.y + (cy + 9.0F) * scale};
}

GameInfoHud::Rect GameInfoHud::faceBoxRect(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const noexcept {
    const auto root = mappedRoot(viewportWidth, viewportHeight);
    const eudoria::ui::LegacyViewport viewport{static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const float scale = viewport.scale();
    constexpr float x = 338.15F;
    constexpr float y = -190.0F;
    constexpr float width = 215.0F;
    constexpr float height = 190.0F;
    return {root.x + x * scale, root.y + y * scale, root.x + (x + width) * scale, root.y + (y + height) * scale};
}

bool GameInfoHud::onMouseMove(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    hoveredSend_ = -1;
    hoveredDisplay_ = -1;
    hoveredEnter_ = enterRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
    hoveredFace_ = faceRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
    for (std::size_t i = 0; i < sendScopes_.size(); ++i) {
        if (sendButtonRect(i, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            hoveredSend_ = static_cast<int>(i);
            break;
        }
    }
    if (contentOffsetX_ >= 0.0F) {
        contentHovered_ = contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
    } else {
        contentHovered_ = hideRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
    }
    if (contentHovered_ && contentOffsetX_ >= 0.0F) {
        for (std::size_t i = 0; i < displayScopes_.size(); ++i) {
            if (displayButtonRect(i, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
                hoveredDisplay_ = static_cast<int>(i);
                break;
            }
        }
    }
    return hoveredSend_ >= 0 || hoveredDisplay_ >= 0 || hoveredEnter_ || hoveredFace_ || contentHovered_ ||
        inputRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
}

bool GameInfoHud::onMouseDown(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    pressedAction_ = PressedAction::None;
    pressedIndex_ = -1;

    if (faceOpen_ && faceBoxRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        const auto local = screenToLegacy(mouseX, mouseY, viewportWidth, viewportHeight);
        constexpr float faceX = 338.15F;
        constexpr float faceY = -190.0F;
        if (local.x > faceX + 190.0F && local.y < faceY + 30.0F) {
            faceOpen_ = false;
            return true;
        }
        if (local.x >= faceX + 10.0F && local.x < faceX + 210.0F && local.y >= faceY + 10.0F && local.y < faceY + 185.0F) {
            const int column = static_cast<int>((local.x - faceX - 10.0F) / 25.0F);
            const int row = static_cast<int>((local.y - faceY - 10.0F) / 25.0F);
            const int index = row * 8 + column;
            if (index >= 0 && index < 50) {
                pressedAction_ = PressedAction::FaceCell;
                pressedIndex_ = index;
                return true;
            }
        }
        return true;
    }

    if (inputRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        inputFocused_ = true;
        dirtyInput_ = true;
        return true;
    }

    for (std::size_t i = 0; i < sendScopes_.size(); ++i) {
        if (sendButtonRect(i, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::SendScope;
            pressedIndex_ = static_cast<int>(i);
            return true;
        }
    }

    if (enterRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::Enter;
        return true;
    }
    if (faceRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
        pressedAction_ = PressedAction::Face;
        return true;
    }

    if (contentHovered_) {
        if (hideRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::Hide;
            return true;
        }
        if (contentOffsetX_ >= 0.0F && resizeRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            pressedAction_ = PressedAction::Resize;
            return true;
        }
        if (contentOffsetX_ >= 0.0F) {
            for (std::size_t i = 0; i < displayScopes_.size(); ++i) {
                if (displayButtonRect(i, viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
                    pressedAction_ = PressedAction::DisplayScope;
                    pressedIndex_ = static_cast<int>(i);
                    return true;
                }
            }
        }
    }

    inputFocused_ = false;
    dirtyInput_ = true;
    return contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY);
}

bool GameInfoHud::onMouseUp(
    const float mouseX,
    const float mouseY,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    bool consumed = false;
    switch (pressedAction_) {
    case PressedAction::SendScope:
        if (pressedIndex_ >= 0 && static_cast<std::size_t>(pressedIndex_) < sendScopes_.size() &&
            sendButtonRect(static_cast<std::size_t>(pressedIndex_), viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            if (pressedIndex_ < 6) setSendScope(sendScopes_[static_cast<std::size_t>(pressedIndex_)].scope);
        }
        consumed = true;
        break;
    case PressedAction::DisplayScope:
        if (pressedIndex_ >= 0 && static_cast<std::size_t>(pressedIndex_) < displayScopes_.size() &&
            displayButtonRect(static_cast<std::size_t>(pressedIndex_), viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            setDisplayScope(displayScopes_[static_cast<std::size_t>(pressedIndex_)].scope);
        }
        consumed = true;
        break;
    case PressedAction::Enter:
        if (enterRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) sendInput();
        consumed = true;
        break;
    case PressedAction::Face:
        if (faceRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) {
            faceOpen_ = !faceOpen_;
            inputFocused_ = true;
            dirtyInput_ = true;
        }
        consumed = true;
        break;
    case PressedAction::Hide:
        if (hideRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) toggleContentHidden();
        consumed = true;
        break;
    case PressedAction::Resize:
        if (resizeRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) toggleContentHeight();
        consumed = true;
        break;
    case PressedAction::FaceCell:
        if (pressedIndex_ >= 0 && pressedIndex_ < 50) insertFaceCode(static_cast<std::size_t>(pressedIndex_));
        consumed = true;
        break;
    case PressedAction::None:
        break;
    }
    pressedAction_ = PressedAction::None;
    pressedIndex_ = -1;
    return consumed;
}

bool GameInfoHud::onMouseWheel(
    const float mouseX,
    const float mouseY,
    const int wheelDelta,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) noexcept {
    if (contentOffsetX_ < 0.0F || !contentRect(viewportWidth, viewportHeight).contains(mouseX, mouseY)) return false;
    messageScroll_ -= static_cast<float>(wheelDelta) / 120.0F * 36.0F;
    clampMessageScroll();
    return true;
}

bool GameInfoHud::onKeyDown(const std::uint32_t virtualKey) noexcept {
    if (!inputFocused_) {
        if (virtualKey == VK_RETURN) {
            inputFocused_ = true;
            dirtyInput_ = true;
            return true;
        }
        return false;
    }

    if (virtualKey == VK_ESCAPE) {
        inputFocused_ = false;
        faceOpen_ = false;
        dirtyInput_ = true;
        return true;
    }
    if (virtualKey == VK_UP) {
        applyHistory(-1);
        return true;
    }
    if (virtualKey == VK_DOWN) {
        applyHistory(1);
        return true;
    }
    return false;
}

bool GameInfoHud::onChar(const wchar_t character) {
    if (!inputFocused_) return false;

    if (character == L'\r' || character == L'\n') {
        sendInput();
        return true;
    }
    if (character == L'\b') {
        if (!input_.empty()) {
            input_.pop_back();
            dirtyInput_ = true;
        }
        return true;
    }
    if (character == 0x1B) {
        return true;
    }
    if (isPrintable(character) && input_.size() < kInputLimit) {
        input_.push_back(character);
        dirtyInput_ = true;
        return true;
    }
    return true;
}

void GameInfoHud::sendInput() {
    if (input_.empty()) return;

    const std::wstring original = input_;
    pushHistory(original);
    input_.clear();
    historyIndex_ = -1;
    dirtyInput_ = true;

    ChatScope scope = sendScope_;
    std::wstring recipient;
    std::wstring body = original;

    if (!original.empty() && original.front() == L'/' && original.find(L' ') != std::wstring::npos) {
        const auto separator = original.find(L' ');
        recipient = original.substr(1, separator - 1);
        body = original.substr(separator + 1);
        scope = ChatScope::Whisper;
        sendScope_ = ChatScope::Whisper;
        input_ = L"/" + recipient + L" ";
        dirtyInput_ = true;
    } else if (sendScope_ == ChatScope::Whisper) {
        sendScope_ = ChatScope::Nearby;
        scope = ChatScope::Nearby;
    }

    if (body.empty()) return;

    ChatMessage message;
    message.scope = scope;
    message.sender = recipient.empty() ? L"You" : (L"You -> " + recipient);
    message.text = body;
    receiveMessage(std::move(message));
}

void GameInfoHud::setSendScope(const ChatScope scope) {
    sendScope_ = scope;
    if (scope != ChatScope::Whisper && !input_.empty() && input_.front() == L'/') {
        input_.clear();
        dirtyInput_ = true;
    }
}

void GameInfoHud::setDisplayScope(const ChatScope scope) {
    if (displayScope_ == scope) return;
    displayScope_ = scope;
    dirtyMessages_ = true;
    rebuildMessages();
}

void GameInfoHud::pushHistory(const std::wstring& value) {
    if (!history_.empty() && history_.back() == value) return;
    history_.push_back(value);
    while (history_.size() > kHistoryLimit) history_.pop_front();
}

void GameInfoHud::applyHistory(const int delta) {
    if (history_.empty()) return;
    if (historyIndex_ < 0) historyIndex_ = static_cast<int>(history_.size());
    historyIndex_ = std::clamp(historyIndex_ + delta, 0, static_cast<int>(history_.size()) - 1);
    input_ = history_[static_cast<std::size_t>(historyIndex_)];
    dirtyInput_ = true;
}

void GameInfoHud::clampMessageScroll() noexcept {
    const float maxScroll = std::max(0.0F, totalMessageHeight_ - infoHeight_);
    messageScroll_ = std::clamp(messageScroll_, 0.0F, maxScroll);
}

void GameInfoHud::toggleContentHidden() noexcept {
    contentOffsetX_ = contentOffsetX_ < 0.0F ? 0.0F : -333.95F;
}

void GameInfoHud::toggleContentHeight() noexcept {
    infoHeight_ = infoHeight_ > kDefaultInfoHeight ? kDefaultInfoHeight : kExpandedInfoHeight;
    clampMessageScroll();
}

void GameInfoHud::insertFaceCode(const std::size_t index) {
    if (input_.size() + 3 > kInputLimit) return;
    wchar_t buffer[4]{};
    swprintf_s(buffer, 4, L"#%02u", static_cast<unsigned>(index));
    input_ += buffer;
    faceOpen_ = false;
    inputFocused_ = true;
    dirtyInput_ = true;
}

void GameInfoHud::drawClipped(
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
    if (!texture.valid() || legacyWidth <= 0.0F || legacyHeight <= 0.0F) return;
    const float top = std::max(legacyY, clipTopLegacy);
    const float bottom = std::min(legacyY + legacyHeight, clipBottomLegacy);
    if (bottom <= top) return;

    const float crop = top - legacyY;
    const float visible = bottom - top;
    renderer.drawRegion(
        texture,
        SpriteSourceRect{
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

} // namespace eudoria::game::ui
