#pragma once

#include <Windows.h>

#include "engine/render/D3D11Renderer.h"
#include "game/ui/LegacyHudReference.h"

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
    game::ui::LegacyHudReference legacyHudReference_;
};

} // namespace eudoria
