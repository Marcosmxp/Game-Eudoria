#pragma once

#include <cstdint>

namespace eudoria::ui {

enum class Anchor {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Center,
};

struct Point {
    float x = 0.0F;
    float y = 0.0F;
};

struct LegacyViewport final {
    static constexpr float kReferenceWidth = 1200.0F;
    static constexpr float kReferenceHeight = 640.0F;

    float width = kReferenceWidth;
    float height = kReferenceHeight;

    // Crystal Saga's SomcGame.initStage() explicitly configures
    // StageScaleMode.NO_SCALE + StageAlign.TOP_LEFT. The Flash HUD therefore
    // stays at native pixel size when the stage/window grows; only edge-anchored
    // roots move with stageWidth/stageHeight. Scaling the HUD here was the cause
    // of blurred icons in fullscreen and incorrect spacing.
    [[nodiscard]] constexpr float scale() const noexcept {
        return 1.0F;
    }

    [[nodiscard]] Point mapRoot(const Point legacy, const Anchor anchor) const noexcept {
        constexpr float s = 1.0F;

        switch (anchor) {
        case Anchor::TopLeft:
            return {legacy.x, legacy.y};
        case Anchor::TopRight:
            return {width - ((kReferenceWidth - legacy.x) * s), legacy.y};
        case Anchor::BottomLeft:
            return {legacy.x, height - ((kReferenceHeight - legacy.y) * s)};
        case Anchor::BottomCenter:
            return {
                (width * 0.5F) + ((legacy.x - (kReferenceWidth * 0.5F)) * s),
                height - ((kReferenceHeight - legacy.y) * s),
            };
        case Anchor::BottomRight:
            return {
                width - ((kReferenceWidth - legacy.x) * s),
                height - ((kReferenceHeight - legacy.y) * s),
            };
        case Anchor::Center:
            return {
                (width * 0.5F) + ((legacy.x - (kReferenceWidth * 0.5F)) * s),
                (height * 0.5F) + ((legacy.y - (kReferenceHeight * 0.5F)) * s),
            };
        }

        return legacy;
    }
};

} // namespace eudoria::ui
