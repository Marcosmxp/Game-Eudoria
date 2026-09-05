#pragma once

#include <algorithm>
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

    [[nodiscard]] float scale() const noexcept {
        if (width <= 0.0F || height <= 0.0F) {
            return 1.0F;
        }
        return std::min(width / kReferenceWidth, height / kReferenceHeight);
    }

    [[nodiscard]] Point mapRoot(const Point legacy, const Anchor anchor) const noexcept {
        const float s = scale();

        switch (anchor) {
        case Anchor::TopLeft:
            return {legacy.x * s, legacy.y * s};
        case Anchor::TopRight:
            return {width - ((kReferenceWidth - legacy.x) * s), legacy.y * s};
        case Anchor::BottomLeft:
            return {legacy.x * s, height - ((kReferenceHeight - legacy.y) * s)};
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
