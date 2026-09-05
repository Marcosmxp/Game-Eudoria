#include <Windows.h>

#include "engine/platform/Win32Application.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    eudoria::Win32Application application;
    return application.run(instance, showCommand);
}
