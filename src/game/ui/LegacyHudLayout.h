#pragma once

#include "engine/ui/LegacyUiTransform.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace eudoria::game::ui {

struct HudRoot final {
    std::string_view id;
    std::string_view legacyClass;
    std::string_view legacyExport;
    std::uint16_t characterId;
    eudoria::ui::Point root;
    eudoria::ui::Anchor anchor;
};

inline constexpr std::array kHudRoots{
    HudRoot{"playerInfo", "playerUI.PlayerInfoUIMC", "symbol3550", 3550, {0.0F, 0.0F}, eudoria::ui::Anchor::TopLeft},
    HudRoot{"gameInfo", "playerUI.GameInfoUIMC", "symbol4343", 4343, {0.0F, 570.0F}, eudoria::ui::Anchor::BottomLeft},
    HudRoot{"controlBar", "playerUI.ControlBarUIMC", "symbol4131", 4131, {600.0F, 640.0F}, eudoria::ui::Anchor::BottomCenter},
    HudRoot{"smallMap", "playerUI.SmallMapUIMC", "symbol1825", 1825, {1200.0F, 0.0F}, eudoria::ui::Anchor::TopRight},
    HudRoot{"taskTracer", "playerUI.TaskTracerUIMC", "symbol4135", 4135, {960.0F, 230.0F}, eudoria::ui::Anchor::TopRight},
};

} // namespace eudoria::game::ui
