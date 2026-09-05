#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"
#include "game/ui/HudWindowManager.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace eudoria::game::ui {

class ControlBar final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/control_bar");

    void render(
        SpriteRenderer& renderer,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight,
        const HudWindowManager& windows) const;

    void onMouseMove(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    void onMouseDown(
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

    bool onKeyUp(std::uint32_t virtualKey, HudWindowManager& windows) noexcept;

    [[nodiscard]] std::string_view hoveredId() const noexcept;
    [[nodiscard]] bool soundEnabled() const noexcept { return soundEnabled_; }
    [[nodiscard]] bool totalMenuExpanded() const noexcept { return totalMenuExpanded_; }

private:
    struct Rect final {
        float left;
        float top;
        float right;
        float bottom;

        [[nodiscard]] bool contains(float x, float y) const noexcept {
            return x >= left && x <= right && y >= top && y <= bottom;
        }
    };

    struct ButtonSpec final {
        std::string_view id;
        std::string_view assetDirectory;
        eudoria::ui::Point placement;
        eudoria::ui::Point imageOffset;
        Rect hitRect;
        HudWindow window;
        std::uint32_t shortcut;
    };

    struct ButtonVisual final {
        SpriteTexture up;
        SpriteTexture over;
        SpriteTexture down;
    };

    [[nodiscard]] static const std::array<ButtonSpec, 13>& buttons() noexcept;
    [[nodiscard]] static const Rect& soundHitRect() noexcept;
    [[nodiscard]] static const Rect& totalMenuToggleHitRect() noexcept;

    [[nodiscard]] static eudoria::ui::Point toLocal(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    [[nodiscard]] static int hitTest(const eudoria::ui::Point local) noexcept;
    static bool activate(std::size_t index, HudWindowManager& windows) noexcept;

    void toggleTotalMenu() noexcept;
    [[nodiscard]] float currentTotalMenuY() const noexcept;
    [[nodiscard]] bool totalMenuShouldRender() const noexcept;
    void renderTotalMenu(
        SpriteRenderer& renderer,
        const eudoria::ui::Point& root,
        float scale) const;

    static constexpr eudoria::ui::Point kRoot{600.0F, 640.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::BottomCenter;

    // symbol4131 first-frame unnamed payload shape: DefineShape2_3993.
    static constexpr eudoria::ui::Point kBaseImageOffset{-584.0F, -67.0F};

    // symbol4131 cmdSoundSwitch (DefineSprite_257).
    static constexpr eudoria::ui::Point kSoundPlacement{-457.95F, -13.55F};
    static constexpr eudoria::ui::Point kSoundImageOffset{-8.05F, -12.45F};

    // symbol4131 up/down controls. Both button exports share the same raster bounds.
    static constexpr eudoria::ui::Point kTotalMenuTogglePlacement{460.5F, -25.5F};
    static constexpr eudoria::ui::Point kTotalMenuToggleImageOffset{-8.5F, -13.5F};

    // symbol4131 totalIcon placement + FFDec raster bounds for symbol4130.
    static constexpr float kTotalMenuX = -18.2F;
    static constexpr float kTotalMenuInitialY = -177.75F;
    static constexpr float kTotalMenuExpandedY = -180.0F;
    static constexpr float kTotalMenuCollapsedY = 0.0F;
    static constexpr float kTotalMenuRasterMinX = -350.8F;
    static constexpr float kTotalMenuRasterMinY = -150.25F;

    // DefineShape4_4039 is a clipping mask at depth 51 for totalIcon at depth 52.
    static constexpr Rect kTotalMenuClip{-507.85F, -325.85F, 513.15F, -70.85F};

    // txt/iss.json has no showGuixuConfig flag, so WorldConfig::getValue resolves false.
    // The legacy constructor therefore hides cmdGuixu. In symbol4130 it is the only
    // content occupying the first ~62 raster rows.
    static constexpr float kTotalMenuConfigCropTop = 62.0F;

    static constexpr auto kTotalMenuTweenDuration = std::chrono::milliseconds{500};

    SpriteTexture baseSkin_;
    SpriteTexture totalMenuSkin_;
    SpriteTexture soundOn_;
    SpriteTexture soundOff_;
    std::array<ButtonVisual, 13> visuals_{};
    ButtonVisual expandVisual_;
    ButtonVisual collapseVisual_;

    int hovered_ = -1;
    int pressed_ = -1;
    bool soundEnabled_ = true;

    bool totalMenuExpanded_ = true;
    bool totalMenuAnimating_ = false;
    float totalMenuTweenStartY_ = kTotalMenuInitialY;
    float totalMenuTweenTargetY_ = kTotalMenuInitialY;
    std::chrono::steady_clock::time_point totalMenuTweenStarted_{};
};

} // namespace eudoria::game::ui
