#include "game/ui/PlayerInfoHud.h"

#include "engine/render/TextRasterizer.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace eudoria::game::ui {
namespace {

void buildTextTexture(
    const SpriteRenderer& renderer,
    const std::wstring& text,
    const std::uint32_t width,
    const std::uint32_t height,
    const std::uint32_t fontHeight,
    const std::uint8_t red,
    const std::uint8_t green,
    const std::uint8_t blue,
    const TextHorizontalAlign align,
    SpriteTexture& target) {
    target = {};

    TextTextureStyle style{};
    style.fontFamily = L"Arial";
    style.fontPixelHeight = fontHeight;
    style.red = red;
    style.green = green;
    style.blue = blue;
    style.alpha = 255;
    style.align = align;
    style.wordWrap = false;
    style.bold = false;

    TextTextureResult result{};
    if (createTextTexture(renderer, text, width, height, style, result)) {
        target = std::move(result.texture);
    }
}

void drawTextTexture(
    SpriteRenderer& renderer,
    const SpriteTexture& texture,
    const eudoria::ui::Point& root,
    const float x,
    const float y,
    const float scale) {
    if (!texture.valid()) {
        return;
    }

    renderer.draw(
        texture,
        root.x + (x * scale),
        root.y + (y * scale),
        static_cast<float>(texture.width) * scale,
        static_cast<float>(texture.height) * scale);
}

} // namespace

bool PlayerInfoHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    bool loadedAny = false;

    const auto load = [&](const std::filesystem::path& relative, SpriteTexture& target) {
        const bool loaded = renderer.loadTexture((runtimeRoot / relative).wstring(), target);
        loadedAny = loadedAny || loaded;
        return loaded;
    };

    loadedAny = loadFrames(renderer, runtimeRoot / L"background", backgroundFrames_) || loadedAny;
    load(L"pet_frame.png", petFrame_);
    load(L"resource_back.png", resourceBack_);
    load(L"resource_back_flip.png", resourceBackFlip_);
    load(L"divider.png", divider_);

    loadedAny = loadFrames(renderer, runtimeRoot / L"reserve_hp", reserveHpFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"reserve_mp", reserveMpFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"reserve_mask", reserveMaskFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"reserve_mask_flip", reserveMaskFlipFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"hp", hpFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"mp", mpFrames_) || loadedAny;

    load(L"fate_skill/up.png", fateSkill_);
    load(L"pet_action/up.png", petAction_);
    loadedAny = loadFrames(renderer, runtimeRoot / L"fps", fpsFrames_) || loadedAny;
    loadedAny = loadFrames(renderer, runtimeRoot / L"ping", pingFrames_) || loadedAny;
    load(L"team_leader.png", teamLeader_);
    load(L"head/default.png", defaultHead_);

    rebuildTextTextures(renderer);
    fpsSampleStarted_ = std::chrono::steady_clock::now();
    return loadedAny;
}

void PlayerInfoHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) {
    updateFpsSample();
    if (textDirty_) {
        rebuildTextTextures(renderer);
    }

    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    // symbol3550 depth 1: bgBox (3505). PlayerInfoUI.setCombat() selects
    // frame 1 or 2; no PlayerInfo composite texture is used at runtime.
    drawFrame(
        renderer,
        backgroundFrames_,
        state_.combat ? 2 : 1,
        root,
        kBackground,
        scale);

    // depth 3: the pet/status lower frame (shape3507).
    drawTexture(renderer, petFrame_, root, kPetFrame, scale);

    // depths 4/6/8/10: HP/MP and pet HP/MP back plates. The MP instances use
    // the original negative scaleX; the extraction script creates exact flipped
    // copies so the renderer can keep positive destination dimensions.
    drawTexture(renderer, resourceBack_, root, kHpBack, scale);
    drawTexture(renderer, resourceBackFlip_, root, kMpBack, scale);
    drawTexture(renderer, resourceBack_, root, kPetHpBack, scale);
    drawTexture(renderer, resourceBackFlip_, root, kPetMpBack, scale);

    // depth 12.
    drawTexture(renderer, divider_, root, kDivider, scale);

    // depths 13/17: reserve/background resource bars.
    drawFrame(
        renderer,
        reserveHpFrames_,
        reserveFrameFor(vitals_.bkHp, vitals_.bkHpMax),
        root,
        kReserveHp,
        scale);
    drawFrame(
        renderer,
        reserveMpFrames_,
        reserveFrameFor(vitals_.bkMp, vitals_.bkMpMax),
        root,
        kReserveMp,
        scale);

    // depths 21/26: primary HP and MP MovieClips.
    drawBar(renderer, hpFrames_, frameFor(vitals_.hp, vitals_.hpMax), kHpBar, root, scale);
    drawBar(renderer, mpFrames_, frameFor(vitals_.mp, vitals_.mpMax), kMpBar, root, scale);

    // depths 33/38: pet resource bars. PlayerInfoUI.updatePetInfo() explicitly
    // resets them to frame 1 when no pet is active.
    drawBar(
        renderer,
        hpFrames_,
        state_.hasPet ? frameFor(vitals_.petHp, vitals_.petHpMax) : 1,
        kPetHpBar,
        root,
        scale);
    drawBar(
        renderer,
        mpFrames_,
        state_.hasPet ? frameFor(vitals_.petMp, vitals_.petMpMax) : 1,
        kPetMpBar,
        root,
        scale);

    // depths 45/49.
    drawFrame(
        renderer,
        reserveHpFrames_,
        state_.hasPet ? reserveFrameFor(vitals_.petBkHp, vitals_.petBkHpMax) : 1,
        root,
        kPetReserveHp,
        scale);
    drawFrame(
        renderer,
        reserveMpFrames_,
        state_.hasPet ? reserveFrameFor(vitals_.petBkMp, vitals_.petBkMpMax) : 1,
        root,
        kPetReserveMp,
        scale);

    // depths 53/57/61/65. PlayerInfoUI constructor initializes all masks to
    // frame 1. Timed mask progression will later be driven by the offline
    // combat-state clock; the visual component and its exact geometry are now
    // already independent.
    drawFrame(renderer, reserveMaskFrames_, 1, root, kMaskHp, scale);
    drawFrame(renderer, reserveMaskFlipFrames_, 1, root, kMaskMp, scale);
    drawFrame(renderer, reserveMaskFrames_, 1, root, kPetMaskHp, scale);
    drawFrame(renderer, reserveMaskFlipFrames_, 1, root, kPetMaskMp, scale);

    // depths 69-72: txtName, txtLevel, txtHp, txtMp. Bounds/colors/font sizes
    // are read from DefineEditText 3520-3523 in assets.swf.
    drawTextTexture(renderer, nameText_, root, 117.0F, 8.0F, scale);
    drawTextTexture(renderer, levelText_, root, 70.0F, 52.5F, scale);
    drawTextTexture(renderer, hpText_, root, 81.95F, 24.0F, scale);
    drawTextTexture(renderer, mpText_, root, 81.95F, 37.0F, scale);

    // depth 73: headIconPoint. The fallback HeadIcon_000 is the actual payload
    // class used by HeadIconCode when a dynamic ta3 portrait is unavailable.
    // Player-specific ta3 portraits can replace this texture without changing
    // symbol3550 geometry.
    drawTexture(renderer, defaultHead_, root, kHeadIcon, scale);

    // depth 75 pointPetIcon is a dynamic GameItemManager child in the legacy
    // client. It intentionally remains empty when state_.hasPet is false.

    // depth 77.
    drawTextTexture(renderer, pkText_, root, 69.0F, 1.0F, scale);

    // depth 78: cmdFateSkill (3529).
    drawTexture(renderer, fateSkill_, root, kFateSkill, scale);

    // depth 81 and pet text fields 83-86.
    drawTexture(renderer, petAction_, root, kPetAction, scale);
    if (state_.hasPet) {
        drawTextTexture(renderer, petActionText_, root, 190.95F, 85.05F, scale);
        drawTextTexture(renderer, petHpText_, root, 90.95F, 74.0F, scale);
        drawTextTexture(renderer, petMpText_, root, 90.95F, 87.35F, scale);
        drawTextTexture(renderer, petLevelText_, root, 83.95F, 100.4F, scale);
    }

    // depths 87/90: fpsIcon and pingIcon. Frame labels recovered from the SWF:
    // 1=fast, 2=medium, 3=slow.
    const int fpsFrame = sampledFps_ <= 15 ? 3 : (sampledFps_ <= 25 ? 2 : 1);
    const int pingFrame = state_.pingMs > 500 ? 3 : (state_.pingMs > 200 ? 2 : 1);
    drawFrame(renderer, fpsFrames_, fpsFrame, root, kFpsIcon, scale);
    drawFrame(renderer, pingFrames_, pingFrame, root, kPingIcon, scale);

    // depth 93: hidden by default in PlayerInfoUI constructor.
    if (state_.teamLeader) {
        drawTexture(renderer, teamLeader_, root, kTeamLeader, scale);
    }
}

int PlayerInfoHud::frameFor(const std::int32_t value, const std::int32_t maximum) noexcept {
    if (maximum <= 0) {
        return 2;
    }

    const double ratio = std::clamp(
        static_cast<double>(value) / static_cast<double>(maximum),
        0.0,
        1.0);
    return std::clamp(static_cast<int>(ratio * 100.0), 2, 100);
}

int PlayerInfoHud::reserveFrameFor(const std::int32_t value, const std::int32_t maximum) noexcept {
    if (maximum <= 0) {
        return 1;
    }

    const double ratio = std::clamp(
        static_cast<double>(value) / static_cast<double>(maximum),
        0.0,
        1.0);
    return std::clamp(static_cast<int>(ratio * 100.0), 1, 100);
}

bool PlayerInfoHud::loadFrames(
    SpriteRenderer& renderer,
    const std::filesystem::path& root,
    const std::span<SpriteTexture> frames) {
    bool loadedAny = false;
    for (std::size_t index = 0; index < frames.size(); ++index) {
        const auto file = root / (std::to_wstring(index + 1) + L".png");
        if (std::filesystem::exists(file) && renderer.loadTexture(file.wstring(), frames[index])) {
            loadedAny = true;
        }
    }
    return loadedAny;
}

void PlayerInfoHud::drawTexture(
    SpriteRenderer& renderer,
    const SpriteTexture& texture,
    const eudoria::ui::Point& root,
    const RasterPlacement& placement,
    const float legacyScale) {
    if (!texture.valid()) {
        return;
    }

    renderer.draw(
        texture,
        root.x + (placement.x * legacyScale),
        root.y + (placement.y * legacyScale),
        static_cast<float>(texture.width) * placement.scaleX * legacyScale,
        static_cast<float>(texture.height) * placement.scaleY * legacyScale);
}

void PlayerInfoHud::drawFrame(
    SpriteRenderer& renderer,
    const std::span<const SpriteTexture> frames,
    const int frame,
    const eudoria::ui::Point& root,
    const RasterPlacement& placement,
    const float legacyScale) {
    if (frames.empty()) {
        return;
    }

    const auto index = static_cast<std::size_t>(
        std::clamp(frame, 1, static_cast<int>(frames.size())) - 1);
    drawTexture(renderer, frames[index], root, placement, legacyScale);
}

void PlayerInfoHud::drawBar(
    SpriteRenderer& renderer,
    const std::array<SpriteTexture, kResourceFrameCount>& frames,
    const int frame,
    const BarPlacement& placement,
    const eudoria::ui::Point& root,
    const float legacyScale) {
    const auto index = static_cast<std::size_t>(std::clamp(frame, 1, 100) - 1);
    const auto& texture = frames[index];
    if (!texture.valid()) {
        return;
    }

    const float localX = placement.position.x + (placement.boundsMinX * placement.scaleX);
    const float localY = placement.position.y + (placement.boundsMinY * placement.scaleY);

    renderer.draw(
        texture,
        root.x + (localX * legacyScale),
        root.y + (localY * legacyScale),
        static_cast<float>(texture.width) * std::abs(placement.scaleX) * legacyScale,
        static_cast<float>(texture.height) * std::abs(placement.scaleY) * legacyScale);
}

void PlayerInfoHud::rebuildTextTextures(const SpriteRenderer& renderer) {
    buildTextTexture(
        renderer,
        state_.name,
        93,
        16,
        10,
        255,
        255,
        255,
        TextHorizontalAlign::Center,
        nameText_);

    buildTextTexture(
        renderer,
        std::to_wstring(state_.level),
        28,
        18,
        12,
        255,
        255,
        255,
        TextHorizontalAlign::Center,
        levelText_);

    buildTextTexture(
        renderer,
        std::to_wstring(vitals_.hp) + L"/" + std::to_wstring(vitals_.hpMax),
        127,
        15,
        9,
        255,
        223,
        223,
        TextHorizontalAlign::Center,
        hpText_);

    buildTextTexture(
        renderer,
        std::to_wstring(vitals_.mp) + L"/" + std::to_wstring(vitals_.mpMax),
        127,
        15,
        9,
        191,
        230,
        255,
        TextHorizontalAlign::Center,
        mpText_);

    buildTextTexture(
        renderer,
        state_.pkMode,
        56,
        16,
        10,
        255,
        255,
        255,
        TextHorizontalAlign::Center,
        pkText_);

    const std::wstring petAction = state_.hasPet
        ? (state_.petPassiveAttack ? L"Attack" : L"Defense")
        : L"";
    buildTextTexture(
        renderer,
        petAction,
        47,
        16,
        10,
        255,
        255,
        0,
        TextHorizontalAlign::Left,
        petActionText_);

    const std::wstring petHp = state_.hasPet
        ? std::to_wstring(vitals_.petHp) + L"/" + std::to_wstring(vitals_.petHpMax)
        : L"";
    const std::wstring petMp = state_.hasPet
        ? std::to_wstring(vitals_.petMp) + L"/" + std::to_wstring(vitals_.petMpMax)
        : L"";
    const std::wstring petLevel = state_.hasPet ? std::to_wstring(state_.petLevel) : L"";

    buildTextTexture(
        renderer,
        petHp,
        100,
        15,
        9,
        255,
        221,
        221,
        TextHorizontalAlign::Center,
        petHpText_);
    buildTextTexture(
        renderer,
        petMp,
        100,
        15,
        9,
        185,
        227,
        255,
        TextHorizontalAlign::Center,
        petMpText_);
    buildTextTexture(
        renderer,
        petLevel,
        23,
        15,
        9,
        255,
        255,
        255,
        TextHorizontalAlign::Center,
        petLevelText_);

    textDirty_ = false;
}

void PlayerInfoHud::updateFpsSample() {
    ++framesSinceSample_;
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - fpsSampleStarted_);
    if (elapsed.count() < 1000) {
        return;
    }

    const double seconds = std::max(0.001, static_cast<double>(elapsed.count()) / 1000.0);
    sampledFps_ = static_cast<std::uint32_t>(
        std::max(0.0, std::round(static_cast<double>(framesSinceSample_) / seconds)));
    framesSinceSample_ = 0;
    fpsSampleStarted_ = now;
}

} // namespace eudoria::game::ui
