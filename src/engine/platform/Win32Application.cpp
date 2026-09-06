#include "engine/platform/Win32Application.h"

#include "engine/core/GameConfig.h"

#include <windowsx.h>

#include <utility>
#include <vector>

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

    // txt/itl.json is the exact task payload passed to TaskManager.init() by the
    // legacy ResLoadModule. During the UI-only phase there is no offline player
    // save/progression state yet, so Current remains empty. Available is fed with
    // a small, deterministic preview selected from the real Tyria Village main
    // quest definitions. The quest names/NPC/map labels themselves are never
    // invented. This bridge is replaced by TaskManager condition evaluation when
    // gameplay systems are implemented.
    if (taskCatalog_.load()) {
        std::vector<game::ui::AvailableTask> availableTasks;
        for (const auto* task : taskCatalog_.starterUiPreview(8)) {
            if (!task) {
                continue;
            }
            availableTasks.push_back({task->id, task->name, task->receiveAt});
        }
        taskTracer_.setAvailableTasks(std::move(availableTasks));
    }

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
            smallMap_.update();
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
        if (smallMap_.onMouseUp(x, y, renderer_.width(), renderer_.height())) {
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
