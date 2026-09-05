#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <cstdint>
#include <filesystem>

namespace eudoria::game::ui {

class HudChrome final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& referenceRoot = "legacy_assets/reference/ui");

    void render(SpriteRenderer& renderer, std::uint32_t viewportWidth, std::uint32_t viewportHeight) const;

private:
    struct Panel final {
        const wchar_t* fileName;
        eudoria::ui::Point root;
        eudoria::ui::Anchor anchor;
        float originX;
        float originY;
        SpriteTexture texture;
    };

    // GameInfo remains static chrome for now. TaskTracer has its own runtime
    // implementation because the original ActionScript supports drag/collapse.
    std::array<Panel, 1> panels_{{
        {L"game_info.reference.png", {0.0F, 570.0F}, eudoria::ui::Anchor::BottomLeft, 10.0F, 348.5F, {}},
    }};
};

} // namespace eudoria::game::ui
