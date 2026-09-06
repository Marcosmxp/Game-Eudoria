#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/render/TextRasterizer.h"
#include "game/ui/HudWindowManager.h"
#include "game/ui/RoleCharacterHud.h"

#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

class RoleWindowHud final {
public:
    enum class Tab : std::uint8_t {
        Character,
        DivineSoul,
        Familiar,
    };

    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/role_window");

    void update(const HudWindowManager& windows) noexcept;
    void render(
        SpriteRenderer& renderer,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const HudWindowManager& windows) const;

    bool onMouseDown(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const HudWindowManager& windows) noexcept;

    bool onMouseMove(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const HudWindowManager& windows) noexcept;

    bool onMouseUp(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        HudWindowManager& windows) noexcept;

    [[nodiscard]] Tab selectedTab() const noexcept { return selectedTab_; }
    [[nodiscard]] bool dragging() const noexcept { return dragging_; }

private:
    struct Rect final {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;

        [[nodiscard]] bool contains(const float x, const float y) const noexcept {
            return x >= left && x <= right && y >= top && y <= bottom;
        }
    };

    enum class PressedAction : std::uint8_t {
        None,
        Close,
        CharacterTab,
        DivineSoulTab,
        FamiliarTab,
        Drag,
    };

    struct ButtonVisual final {
        SpriteTexture up;
        SpriteTexture over;
        SpriteTexture down;
    };

    [[nodiscard]] Rect backgroundRect() const noexcept;
    [[nodiscard]] Rect closeRect() const noexcept;
    [[nodiscard]] Rect tabRect(Tab tab) const noexcept;

    [[nodiscard]] static float tabX(Tab tab) noexcept;
    [[nodiscard]] PressedAction hitAction(float localX, float localY) const noexcept;
    [[nodiscard]] static Tab tabForAction(PressedAction action) noexcept;
    [[nodiscard]] const SpriteTexture* closeState() const noexcept;

    void drawTab(
        SpriteRenderer& renderer,
        Tab tab,
        const TextTextureResult& label) const;

    // PlayerBoxUI constructor: x = stageWidth / 2; y = stageHeight / 2.
    // The original object does not register an Event.RESIZE listener, so this
    // root is intentionally not re-anchored when F11 later changes the stage.
    static constexpr float kInitialRootX = 600.0F;
    static constexpr float kInitialRootY = 320.0F;

    // symbol4930 / PlayerBoxUIMC exact first-frame display-list geometry.
    static constexpr float kBackgroundPlacementX = 4.0F;
    static constexpr float kBackgroundPlacementY = 0.0F;
    static constexpr float kBackgroundScaleX = 3.4275970458984375F;
    static constexpr float kBackgroundScaleY = 5.8653564453125F;
    static constexpr float kBackgroundBoundsLeft = -66.5F;
    static constexpr float kBackgroundBoundsTop = -52.0F;
    static constexpr float kBackgroundWidth = 133.0F;
    static constexpr float kBackgroundHeight = 104.0F;

    static constexpr float kTitlePlacementX = 0.95F;
    static constexpr float kTitlePlacementY = -297.1F;
    static constexpr float kTitleScaleX = 1.949066162109375F;
    static constexpr float kTitleScaleY = 1.0F;
    static constexpr float kTitleBoundsLeft = -56.0F;
    static constexpr float kTitleBoundsTop = -22.0F;
    static constexpr float kTitleWidth = 111.0F;
    static constexpr float kTitleHeight = 44.0F;

    // DefineEditText 4929 at (-114,-300), bounds (-2..230,-2..17.85).
    static constexpr float kTitleTextX = -116.0F;
    static constexpr float kTitleTextY = -302.0F;
    static constexpr float kTitleTextWidth = 232.0F;
    static constexpr float kTitleTextHeight = 20.0F;

    // pointChildUI is depth 11 in symbol4930. The Character child
    // PlayerFullInfoUIMC is mounted exactly here by PlayerBoxUI.
    static constexpr float kChildPointX = 0.0F;
    static constexpr float kChildPointY = -39.0F;

    // TableButton character436. Shape432 = normal, shape435 = active.
    static constexpr float kCharacterTabX = -215.4F;
    static constexpr float kDivineSoulTabX = -144.4F;
    static constexpr float kFamiliarTabX = -72.2F;
    static constexpr float kTabY = -274.25F;
    static constexpr float kTabScaleX = 1.4256744384765625F;
    static constexpr float kTabScaleY = 1.0F;
    static constexpr float kTabNormalBoundsX = 1.0F;
    static constexpr float kTabNormalBoundsY = 1.0F;
    static constexpr float kTabNormalWidth = 49.1F;
    static constexpr float kTabNormalHeight = 17.0F;
    static constexpr float kTabActiveBoundsX = 0.0F;
    static constexpr float kTabActiveBoundsY = 0.0F;
    static constexpr float kTabActiveWidth = 50.75F;
    static constexpr float kTabActiveHeight = 18.0F;
    static constexpr float kTabLabelWidth = 64.0F;
    static constexpr float kTabLabelHeight = 18.0F;

    // closeButton character172 at (207.7,-291). Its hit-test shape is
    // character171 with bounds -11..11 on both axes.
    static constexpr float kCloseX = 207.7F;
    static constexpr float kCloseY = -291.0F;
    static constexpr float kCloseHitLeft = -11.0F;
    static constexpr float kCloseHitTop = -11.0F;
    static constexpr float kCloseHitSize = 22.0F;
    static constexpr float kCloseRasterOffsetX = -11.0F;
    static constexpr float kCloseRasterOffsetY = -11.0F;

    SpriteTexture background_;
    SpriteTexture titleBox_;
    SpriteTexture tabNormal_;
    SpriteTexture tabActive_;
    ButtonVisual close_;

    TextTextureResult titleLabel_;
    TextTextureResult characterLabel_;
    TextTextureResult divineSoulLabel_;
    TextTextureResult familiarLabel_;

    RoleCharacterHud character_;

    float rootX_ = kInitialRootX;
    float rootY_ = kInitialRootY;
    float dragOffsetX_ = 0.0F;
    float dragOffsetY_ = 0.0F;

    Tab selectedTab_ = Tab::Character;
    PressedAction pressed_ = PressedAction::None;
    PressedAction hovered_ = PressedAction::None;
    bool dragging_ = false;
    bool wasVisible_ = false;
};

} // namespace eudoria::game::ui
