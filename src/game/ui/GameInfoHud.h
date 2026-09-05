#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/render/TextRasterizer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace eudoria::game::ui {

enum class ChatScope : std::uint8_t {
    All,
    Nearby,
    Whisper,
    Team,
    Family,
    World,
    Trade,
    System,
};

struct ChatMessage final {
    ChatScope scope = ChatScope::System;
    std::wstring sender;
    std::wstring text;
};

class GameInfoHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui",
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/game_info");

    void update();
    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    void receiveMessage(ChatMessage message);

    bool onMouseMove(float mouseX, float mouseY, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
    bool onMouseDown(float mouseX, float mouseY, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
    bool onMouseUp(float mouseX, float mouseY, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
    bool onMouseWheel(float mouseX, float mouseY, int wheelDelta, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
    bool onKeyDown(std::uint32_t virtualKey) noexcept;
    bool onChar(wchar_t character);

    [[nodiscard]] bool inputFocused() const noexcept { return inputFocused_; }

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

    struct ScopeButton final {
        ChatScope scope;
        float centerX;
        std::wstring label;
        bool displayButton;
        TextTextureResult normalLabel;
        TextTextureResult activeLabel;
    };

    struct ButtonStates final {
        SpriteTexture up;
        SpriteTexture over;
        SpriteTexture down;
    };

    struct MessageVisual final {
        ChatScope scope = ChatScope::System;
        TextTextureResult text;
        float y = 0.0F;
        float height = 0.0F;
    };

    enum class PressedAction : std::uint8_t {
        None,
        SendScope,
        DisplayScope,
        Enter,
        Face,
        Hide,
        Resize,
        FaceCell,
    };

    [[nodiscard]] eudoria::ui::Point mappedRoot(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] eudoria::ui::Point screenToLegacy(float mouseX, float mouseY, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;

    [[nodiscard]] Rect inputRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect contentRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect sendButtonRect(std::size_t index, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect displayButtonRect(std::size_t index, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect enterRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect faceRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect hideRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect resizeRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;
    [[nodiscard]] Rect faceBoxRect(std::uint32_t viewportWidth, std::uint32_t viewportHeight) const noexcept;

    void rebuildInputTexture();
    void rebuildMessages();
    void rebuildScopeLabels();
    void trimMessages();
    void sendInput();
    void setSendScope(ChatScope scope);
    void setDisplayScope(ChatScope scope);
    void pushHistory(const std::wstring& value);
    void applyHistory(int delta);
    void clampMessageScroll() noexcept;
    void toggleContentHidden() noexcept;
    void toggleContentHeight() noexcept;
    void insertFaceCode(std::size_t index);

    [[nodiscard]] bool messageVisible(ChatScope scope) const noexcept;
    [[nodiscard]] static TextTextureStyle scopeTextStyle(bool active);
    [[nodiscard]] static TextTextureStyle messageTextStyle(ChatScope scope);

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

    static constexpr eudoria::ui::Point kRoot{0.0F, 570.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::BottomLeft;

    // GameInfoUI::onResize => y = stageHeight - 70.
    static constexpr float kReferenceOriginX = 10.0F;
    static constexpr float kReferenceOriginY = 348.5F;
    static constexpr float kDefaultInfoHeight = 183.0F;
    static constexpr float kExpandedInfoHeight = 300.0F;
    static constexpr float kContentBaseOffset = 77.0F;
    static constexpr float kMessageX = 19.0F;
    static constexpr float kMessageLocalY = 27.0F;
    static constexpr float kMessageWidth = 312.0F;
    static constexpr float kScopeButtonWidth = 50.0F;
    static constexpr float kScopeButtonHeight = 21.0F;
    static constexpr float kSendButtonY = -37.55F;
    static constexpr float kReceiveButtonLocalY = 10.15F;
    static constexpr float kInputX = 7.0F;
    static constexpr float kInputY = -23.0F;
    static constexpr float kInputWidth = 267.0F;
    static constexpr float kInputHeight = 21.0F;
    static constexpr std::size_t kMessageLimit = 100;
    static constexpr std::size_t kInputLimit = 180;
    static constexpr std::size_t kHistoryLimit = 20;

    SpriteRenderer* renderer_ = nullptr;
    SpriteTexture skin_;
    SpriteTexture scopeNormal_;
    SpriteTexture scopeActive_;
    ButtonStates enterButton_;
    ButtonStates faceButton_;
    ButtonStates hideButton_;

    std::array<ScopeButton, 7> sendScopes_{};
    std::array<ScopeButton, 6> displayScopes_{};
    std::vector<ChatMessage> messages_;
    std::vector<MessageVisual> messageVisuals_;
    std::deque<std::wstring> history_;
    TextTextureResult inputTexture_;
    std::array<TextTextureResult, 50> faceLabels_{};

    ChatScope sendScope_ = ChatScope::Nearby;
    ChatScope displayScope_ = ChatScope::All;
    std::wstring input_;
    float infoHeight_ = kDefaultInfoHeight;
    float contentOffsetX_ = 0.0F;
    float messageScroll_ = 0.0F;
    float totalMessageHeight_ = 0.0F;
    int historyIndex_ = -1;
    int hoveredSend_ = -1;
    int hoveredDisplay_ = -1;
    int pressedIndex_ = -1;
    PressedAction pressedAction_ = PressedAction::None;
    bool inputFocused_ = false;
    bool hoveredEnter_ = false;
    bool hoveredFace_ = false;
    bool contentHovered_ = false;
    bool faceOpen_ = false;
    bool dirtyMessages_ = true;
    bool dirtyInput_ = true;
};

} // namespace eudoria::game::ui
