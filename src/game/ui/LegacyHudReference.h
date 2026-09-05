#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <filesystem>

namespace eudoria::game::ui {

class LegacyHudReference final {
public:
    bool initialize(SpriteRenderer& renderer, const std::filesystem::path& root = "legacy_assets/reference/ui");
    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;
    void toggle() noexcept { enabled_ = !enabled_; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

private:
    struct Entry final {
        const wchar_t* fileName;
        eudoria::ui::Point root;
        eudoria::ui::Anchor anchor;
        float originX;
        float originY;
        SpriteTexture texture;
    };

    std::array<Entry, 5> entries_{{
        {L"player_info.reference.png", {0.0F, 0.0F}, eudoria::ui::Anchor::TopLeft, 107.0F, 20.5F, {}},
        {L"game_info.reference.png", {0.0F, 570.0F}, eudoria::ui::Anchor::BottomLeft, 10.0F, 348.5F, {}},
        {L"control_bar.reference.png", {600.0F, 640.0F}, eudoria::ui::Anchor::BottomCenter, 586.0F, 332.0F, {}},
        {L"small_map.reference.png", {1200.0F, 0.0F}, eudoria::ui::Anchor::TopRight, 981.0F, 8.5F, {}},
        {L"task_tracer.reference.png", {960.0F, 230.0F}, eudoria::ui::Anchor::TopRight, 2.0F, 38.0F, {}},
    }};

    bool enabled_ = false;
};

} // namespace eudoria::game::ui
