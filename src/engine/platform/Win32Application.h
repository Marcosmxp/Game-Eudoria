#pragma once

#include <Windows.h>

#include "engine/render/D3D11Renderer.h"
#include "game/ui/ControlBar.h"
#include "game/ui/GameInfoHud.h"
#include "game/ui/HudWindowManager.h"
#include "game/ui/LegacyHudReference.h"
#include "game/ui/PlayerInfoHud.h"
#include "game/ui/SmallMapHud.h"
#include "game/ui/TaskTracerHud.h"

namespace eudoria {

class Win32Application final {
public:
    int run(HINSTANCE instance, int showCommand);

private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    bool createMainWindow(HINSTANCE instance, int showCommand);
    void toggleBorderlessFullscreen();

    HWND window_ = nullptr;
    WINDOWPLACEMENT windowPlacement_{sizeof(WINDOWPLACEMENT)};
    D3D11Renderer renderer_;
    game::ui::HudWindowManager hudWindows_;
    game::ui::PlayerInfoHud playerInfo_;
    game::ui::GameInfoHud gameInfo_;
    game::ui::ControlBar controlBar_;
    game::ui::SmallMapHud smallMap_;
    game::ui::TaskTracerHud taskTracer_;
    game::ui::LegacyHudReference legacyHudReference_;
};

} // namespace eudoria
