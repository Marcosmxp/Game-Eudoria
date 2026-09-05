#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"
#include "game/ui/HudWindowManager.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace eudoria::game::ui {

class ControlBar final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui",
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
        SpriteTexture over;
        SpriteTexture down;
    };

    [[nodiscard]] static const std::array<ButtonSpec, 13>& buttons() noexcept;
    [[nodiscard]] static const Rect& soundHitRect() noexcept;

    [[nodiscard]] static eudoria::ui::Point toLocal(
        float mouseX,
        float mouseY,
        std::uint32_t viewportWidth,
        std::uint32_t viewportHeight) noexcept;

    [[nodiscard]] static int hitTest(const eudoria::ui::Point local) noexcept;
    static bool activate(std::size_t index, HudWindowManager& windows) noexcept;

    static constexpr eudoria::ui::Point kRoot{600.0F, 640.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::BottomCenter;

    // Bounds of the FFDec rasterized ControlBarUIMC reference relative to the
    // SWF instance origin. This is temporary composition scaffolding; buttons
    // already use their original independent Flash states below.
    static constexpr float kReferenceOriginX = 586.0F;
    static constexpr float kReferenceOriginY = 332.0F;

    SpriteTexture referenceSkin_;
    std::array<ButtonVisual, 13> visuals_{};
    int hovered_ = -1;
    int pressed_ = -1;
    bool soundEnabled_ = true;
};

} // namespace eudoria::game::ui
