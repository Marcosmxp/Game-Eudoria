#pragma once

#include "engine/render/SpriteRenderer.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace eudoria {

enum class TextHorizontalAlign : std::uint8_t {
    Left,
    Center,
    Right,
};

struct TextTextureStyle final {
    std::wstring fontFamily = L"Arial";
    std::uint32_t fontPixelHeight = 10;
    std::uint8_t red = 255;
    std::uint8_t green = 255;
    std::uint8_t blue = 255;
    std::uint8_t alpha = 255;
    TextHorizontalAlign align = TextHorizontalAlign::Left;
    bool wordWrap = true;
    bool bold = false;
};

struct TextTextureResult final {
    SpriteTexture texture;
    std::uint32_t contentHeight = 0;
};

[[nodiscard]] bool createTextTexture(
    const SpriteRenderer& renderer,
    std::wstring_view text,
    std::uint32_t width,
    std::uint32_t maxHeight,
    const TextTextureStyle& style,
    TextTextureResult& result);

} // namespace eudoria
