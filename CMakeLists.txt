cmake_minimum_required(VERSION 3.24)

project(Eudoria VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Eudoria currently targets Windows only.")
endif()

add_executable(Eudoria WIN32
    src/main.cpp
    src/engine/platform/Win32Application.cpp
    src/engine/render/D3D11Renderer.cpp
    src/engine/render/SpriteRenderer.cpp
    src/game/ui/ControlBar.cpp
    src/game/ui/HudChrome.cpp
    src/game/ui/LegacyHudReference.cpp
    src/game/ui/PlayerInfoHud.cpp
    src/game/ui/SmallMapHud.cpp
)

target_compile_features(Eudoria PRIVATE cxx_std_23)
target_compile_definitions(Eudoria PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(Eudoria PRIVATE src)
target_link_libraries(Eudoria PRIVATE
    d3d11
    dxgi
    d3dcompiler
    windowscodecs
    ole32
)

if(MSVC)
    target_compile_options(Eudoria PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Eudoria)
set_target_properties(Eudoria PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
