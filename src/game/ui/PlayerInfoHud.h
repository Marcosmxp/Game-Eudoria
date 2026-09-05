#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

struct PlayerVitals final {
    std::int32_t hp = 100;
    std::int32_t hpMax = 100;
    std::int32_t mp = 100;
    std::int32_t mpMax = 100;
    std::int32_t petHp = 100;
    std::int32_t petHpMax = 100;
    std::int32_t petMp = 100;
    std::int32_t petMpMax = 100;
};

class PlayerInfoHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui",
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/player_info");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

    void setVitals(const PlayerVitals& vitals) noexcept { vitals_ = vitals; }
    [[nodiscard]] const PlayerVitals& vitals() const noexcept { return vitals_; }

private:
    struct BarPlacement final {
        eudoria::ui::Point position;
        float scaleX;
        float scaleY;
        float boundsMinX;
        float boundsMinY;
    };

    static constexpr std::size_t kFrameCount = 100;

    static int frameFor(std::int32_t value, std::int32_t maximum) noexcept;

    static void drawBar(
        SpriteRenderer& renderer,
        const std::array<SpriteTexture, kFrameCount>& frames,
        int frame,
        const BarPlacement& placement,
        const eudoria::ui::Point& root,
        float legacyScale);

    static bool loadFrames(
        SpriteRenderer& renderer,
        const std::filesystem::path& root,
        std::array<SpriteTexture, kFrameCount>& frames);

    static constexpr eudoria::ui::Point kRoot{0.0F, 0.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopLeft;

    // FFDec raster bounds for PlayerInfoUIMC. The actual SWF origin is inside
    // this bitmap; keeping the offset lets us place the original skin at the
    // same logical root used by PlayerInfoUI.
    static constexpr float kReferenceOriginX = 107.0F;
    static constexpr float kReferenceOriginY = 20.5F;

    // Exact first-frame placements from symbol3550 and recursive SWF bounds
    // for the animated bar symbols (674 = HP, 269 = MP).
    static constexpr BarPlacement kHpBar{{145.0F, 31.0F}, 0.8667755F, 0.78125F, -250.49958F, -12.49652F};
    static constexpr BarPlacement kMpBar{{145.0F, 44.5F}, 0.8662415F, 0.8125F, -79.55006F, -15.0F};
    static constexpr BarPlacement kPetHpBar{{140.35F, 81.55F}, 0.666626F, 0.8125F, -250.49958F, -12.49652F};
    static constexpr BarPlacement kPetMpBar{{140.2F, 94.35F}, 0.658356F, 0.787506F, -79.55006F, -15.0F};

    SpriteTexture referenceSkin_;
    std::array<SpriteTexture, kFrameCount> hpFrames_{};
    std::array<SpriteTexture, kFrameCount> mpFrames_{};
    PlayerVitals vitals_{};
};

} // namespace eudoria::game::ui
