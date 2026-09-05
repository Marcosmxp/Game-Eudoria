#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace eudoria::game::ui {

struct PlayerVitals final {
    std::int32_t hp = 100;
    std::int32_t hpMax = 100;
    std::int32_t mp = 100;
    std::int32_t mpMax = 100;

    std::int32_t bkHp = 1;
    std::int32_t bkHpMax = 100;
    std::int32_t bkMp = 1;
    std::int32_t bkMpMax = 100;

    std::int32_t petHp = 0;
    std::int32_t petHpMax = 100;
    std::int32_t petMp = 0;
    std::int32_t petMpMax = 100;
    std::int32_t petBkHp = 1;
    std::int32_t petBkHpMax = 100;
    std::int32_t petBkMp = 1;
    std::int32_t petBkMpMax = 100;
};

struct PlayerInfoState final {
    std::wstring name = L"Eudoria";
    std::wstring pkMode = L"Pk";
    std::int32_t level = 1;
    std::int32_t petLevel = 1;
    std::int32_t pingMs = 0;
    bool hasPet = false;
    bool petPassiveAttack = true;
    bool teamLeader = false;
    bool combat = false;
};

class PlayerInfoHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/player_info");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight);

    void setVitals(const PlayerVitals& vitals) noexcept {
        vitals_ = vitals;
        textDirty_ = true;
    }
    [[nodiscard]] const PlayerVitals& vitals() const noexcept { return vitals_; }

    void setState(const PlayerInfoState& state) {
        state_ = state;
        textDirty_ = true;
    }
    [[nodiscard]] const PlayerInfoState& state() const noexcept { return state_; }

private:
    struct BarPlacement final {
        eudoria::ui::Point position;
        float scaleX;
        float scaleY;
        float boundsMinX;
        float boundsMinY;
    };

    struct RasterPlacement final {
        float x;
        float y;
        float scaleX = 1.0F;
        float scaleY = 1.0F;
    };

    static constexpr std::size_t kResourceFrameCount = 100;

    static int frameFor(std::int32_t value, std::int32_t maximum) noexcept;
    static int reserveFrameFor(std::int32_t value, std::int32_t maximum) noexcept;

    static bool loadFrames(
        SpriteRenderer& renderer,
        const std::filesystem::path& root,
        std::span<SpriteTexture> frames);

    static void drawTexture(
        SpriteRenderer& renderer,
        const SpriteTexture& texture,
        const eudoria::ui::Point& root,
        const RasterPlacement& placement,
        float legacyScale);

    static void drawFrame(
        SpriteRenderer& renderer,
        std::span<const SpriteTexture> frames,
        int frame,
        const eudoria::ui::Point& root,
        const RasterPlacement& placement,
        float legacyScale);

    static void drawBar(
        SpriteRenderer& renderer,
        const std::array<SpriteTexture, kResourceFrameCount>& frames,
        int frame,
        const BarPlacement& placement,
        const eudoria::ui::Point& root,
        float legacyScale);

    void rebuildTextTextures(const SpriteRenderer& renderer);
    void updateFpsSample();

    static constexpr eudoria::ui::Point kRoot{0.0F, 0.0F};
    static constexpr eudoria::ui::Anchor kAnchor = eudoria::ui::Anchor::TopLeft;

    // Exact first-frame placements from symbol3550 and recursive SWF bounds.
    static constexpr BarPlacement kHpBar{{145.0F, 31.0F}, 0.8667755F, 0.78125F, -250.49958F, -12.49652F};
    static constexpr BarPlacement kMpBar{{145.0F, 44.5F}, 0.8662415F, 0.8125F, -79.55006F, -15.0F};
    static constexpr BarPlacement kPetHpBar{{140.35F, 81.55F}, 0.666626F, 0.8125F, -250.49958F, -12.49652F};
    static constexpr BarPlacement kPetMpBar{{140.2F, 94.35F}, 0.658356F, 0.787506F, -79.55006F, -15.0F};

    // Display-list children that are exported as independent raster assets.
    static constexpr RasterPlacement kBackground{-20.0F, -20.0F};
    static constexpr RasterPlacement kPetFrame{49.0F, 69.0F};
    static constexpr RasterPlacement kHpBack{118.95F, 53.2F};
    static constexpr RasterPlacement kMpBack{160.1F, 53.0F};
    static constexpr RasterPlacement kPetHpBack{105.0F, 102.2F};
    static constexpr RasterPlacement kPetMpBack{147.0F, 102.0F};
    static constexpr RasterPlacement kDivider{71.75F, 0.0F};

    static constexpr RasterPlacement kReserveHp{117.7F, 41.9535F};
    static constexpr RasterPlacement kReserveMp{120.0996F, 41.9535F};
    static constexpr RasterPlacement kPetReserveHp{104.3F, 91.3035F};
    static constexpr RasterPlacement kPetReserveMp{106.6996F, 91.3035F};

    static constexpr RasterPlacement kMaskHp{117.25F, 44.3035F};
    static constexpr RasterPlacement kMaskMp{116.0333F, 44.3035F, 0.9792175F, 1.0F};
    static constexpr RasterPlacement kPetMaskHp{103.7F, 93.8535F};
    static constexpr RasterPlacement kPetMaskMp{102.4833F, 93.8535F, 0.9792175F, 1.0F};

    static constexpr RasterPlacement kHeadIcon{4.0F, 4.0F};
    static constexpr RasterPlacement kFateSkill{171.0F, 15.25F};
    static constexpr RasterPlacement kPetAction{190.95F, 86.35F};
    static constexpr RasterPlacement kFpsIcon{144.5F, -1.5F};
    static constexpr RasterPlacement kPingIcon{149.5F, -1.5F};
    static constexpr RasterPlacement kTeamLeader{0.0F, 0.0F};

    std::array<SpriteTexture, 2> backgroundFrames_{};
    SpriteTexture petFrame_;
    SpriteTexture resourceBack_;
    SpriteTexture resourceBackFlip_;
    SpriteTexture divider_;

    std::array<SpriteTexture, kResourceFrameCount> reserveHpFrames_{};
    std::array<SpriteTexture, kResourceFrameCount> reserveMpFrames_{};
    std::array<SpriteTexture, kResourceFrameCount> reserveMaskFrames_{};
    std::array<SpriteTexture, kResourceFrameCount> reserveMaskFlipFrames_{};
    std::array<SpriteTexture, kResourceFrameCount> hpFrames_{};
    std::array<SpriteTexture, kResourceFrameCount> mpFrames_{};

    SpriteTexture fateSkill_;
    SpriteTexture petAction_;
    std::array<SpriteTexture, 3> fpsFrames_{};
    std::array<SpriteTexture, 3> pingFrames_{};
    SpriteTexture teamLeader_;
    SpriteTexture defaultHead_;

    SpriteTexture nameText_;
    SpriteTexture levelText_;
    SpriteTexture hpText_;
    SpriteTexture mpText_;
    SpriteTexture pkText_;
    SpriteTexture petActionText_;
    SpriteTexture petHpText_;
    SpriteTexture petMpText_;
    SpriteTexture petLevelText_;

    PlayerVitals vitals_{};
    PlayerInfoState state_{};
    bool textDirty_ = true;

    std::uint32_t sampledFps_ = 60;
    std::uint32_t framesSinceSample_ = 0;
    std::chrono::steady_clock::time_point fpsSampleStarted_ = std::chrono::steady_clock::now();
};

} // namespace eudoria::game::ui
