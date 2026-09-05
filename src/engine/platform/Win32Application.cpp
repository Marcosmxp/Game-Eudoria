#include "engine/platform/Win32Application.h"

#include "engine/core/GameConfig.h"

#include <windowsx.h>

namespace eudoria {
namespace {
constexpr wchar_t kWindowClassName[] = L"EudoriaWindowClass";
}

int Win32Application::run(const HINSTANCE instance, const int showCommand) {
    if (!createMainWindow(instance, showCommand)) {
        return 1;
    }

    RECT clientRect{};
    GetClientRect(window_, &clientRect);
    if (!renderer_.initialize(
            window_,
            static_cast<std::uint32_t>(clientRect.right - clientRect.left),
            static_cast<std::uint32_t>(clientRect.bottom - clientRect.top))) {
        MessageBoxW(window_, L"Direct3D 11 initialization failed.", config::kWindowTitle, MB_ICONERROR | MB_OK);
        return 2;
    }

    playerInfo_.initialize(renderer_.sprites());
    gameInfo_.initialize(renderer_.sprites());
    controlBar_.initialize(renderer_.sprites());
    smallMap_.initialize(renderer_.sprites());
    taskTracer_.initialize(renderer_.sprites());

    // Temporary pre-alpha fixture used only to exercise the reconstructed UI
    // until the offline TaskManager is connected. UI geometry and behavior still
    // come from the legacy SWF/ActionScript payload, not screenshots.
    taskTracer_.setTrackedTasks({
        {1001, 1, L"Eudoria reconstruction", L"TaskTracer native runtime active.", true},
        {1002, 2, L"Legacy UI payload", L"HUD geometry is reproduced from assets.swf and ActionScript.", true},
        {1003, 3, L"Offline game systems", L"Connect the local quest manager to replace this development fixture.", false},
    });
    taskTracer_.setAvailableTasks({
        {2001, L"Available task fixture", L"Offline TaskManager pending"},
        {2002, L"Payload integration", L"Local quest data source pending"},
    });

    legacyHudReference_.initialize(renderer_.sprites());

    MSG message{};
    bool running = true;
    while (running) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (running && !IsIconic(window_)) {
            gameInfo_.update();
            taskTracer_.update();

            renderer_.beginFrame();
            playerInfo_.render(renderer_.sprites(), renderer_.width(), renderer_.height());
            gameInfo_.render(renderer_.sprites(), renderer_.width(), renderer_.height());
            controlBar_.render(renderer_.sprites(), renderer_.width(), renderer_.height(), hudWindows_);
            smallMap_.render(renderer_.sprites(), renderer_.width(), renderer_.height());
            taskTracer_.render(renderer_.sprites(), renderer_.width(), renderer_.height());
            legacyHudReference_.render(renderer_.sprites(), renderer_.width(), renderer_.height());
            renderer_.endFrame();
        }
    }

    renderer_.shutdown();
    return static_cast<int>(message.wParam);
}

bool Win32Application::createMainWindow(const HINSTANCE instance, const int showCommand) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Win32Application::windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    RECT windowRect{0, 0, static_cast<LONG>(config::kInitialWindowWidth), static_cast<LONG>(config::kInitialWindowHeight)};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    window_ = CreateWindowExW(
        0,
        kWindowClassName,
        config::kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (!window_) {
        return false;
    }

    ShowWindow(window_, showCommand);
    UpdateWindow(window_);
    return true;
}

LRESULT CALLBACK Win32Application::windowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    Win32Application* application = nullptr;

    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        application = static_cast<Win32Application*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        application->window_ = window;
    } else {
        application = reinterpret_cast<Win32Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (application) {
        return application->handleMessage(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Win32Application::handleMessage(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        renderer_.resize(static_cast<std::uint32_t>(LOWORD(lParam)), static_cast<std::uint32_t>(HIWORD(lParam)));
        taskTracer_.onViewportChanged();
        return 0;

    case WM_MOUSEMOVE: {
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        gameInfo_.onMouseMove(x, y, renderer_.width(), renderer_.height());
        taskTracer_.onMouseMove(x, y, renderer_.width(), renderer_.height());
        controlBar_.onMouseMove(x, y, renderer_.width(), renderer_.height());
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(window);
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        if (gameInfo_.onMouseDown(x, y, renderer_.width(), renderer_.height())) {
            return 0;
        }
        if (taskTracer_.onMouseDown(x, y, renderer_.width(), renderer_.height())) {
            return 0;
        }
        controlBar_.onMouseDown(x, y, renderer_.width(), renderer_.height());
        return 0;
    }

    case WM_LBUTTONUP: {
        if (GetCapture() == window) {
            ReleaseCapture();
        }
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        if (gameInfo_.onMouseUp(x, y, renderer_.width(), renderer_.height())) {
            return 0;
        }
        if (taskTracer_.onMouseUp(x, y, renderer_.width(), renderer_.height(), hudWindows_)) {
            return 0;
        }
        controlBar_.onMouseUp(x, y, renderer_.width(), renderer_.height(), hudWindows_);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(window, &clientPoint);
        if (gameInfo_.onMouseWheel(
                static_cast<float>(clientPoint.x),
                static_cast<float>(clientPoint.y),
                GET_WHEEL_DELTA_WPARAM(wParam),
                renderer_.width(),
                renderer_.height())) {
            return 0;
        }
        if (taskTracer_.onMouseWheel(
                static_cast<float>(clientPoint.x),
                static_cast<float>(clientPoint.y),
                GET_WHEEL_DELTA_WPARAM(wParam),
                renderer_.width(),
                renderer_.height())) {
            return 0;
        }
        break;
    }

    case WM_CHAR:
        if (gameInfo_.onChar(static_cast<wchar_t>(wParam))) {
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (gameInfo_.onKeyDown(static_cast<std::uint32_t>(wParam))) {
            return 0;
        }
        if (wParam == VK_F2) {
            legacyHudReference_.toggle();
            return 0;
        }
        if (wParam == VK_F11) {
            toggleBorderlessFullscreen();
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            PostMessageW(window, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_KEYUP:
        // GameInfoUI stops keyboard propagation while the chat input owns focus.
        if (gameInfo_.inputFocused()) {
            return 0;
        }
        if (controlBar_.onKeyUp(static_cast<std::uint32_t>(wParam), hudWindows_)) {
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

void Win32Application::toggleBorderlessFullscreen() {
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_STYLE));
    if ((style & WS_OVERLAPPEDWINDOW) != 0) {
        MONITORINFO monitorInfo{sizeof(MONITORINFO)};
        if (GetWindowPlacement(window_, &windowPlacement_) &&
            GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
            SetWindowLongPtrW(window_, GWL_STYLE, style & ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW));
            SetWindowPos(
                window_,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLongPtrW(window_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window_, &windowPlacement_);
        SetWindowPos(
            window_,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

} // namespace eudoria
