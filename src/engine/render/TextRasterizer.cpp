#include "engine/render/TextRasterizer.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace eudoria {
namespace {

UINT drawFlags(const TextTextureStyle& style) noexcept {
    UINT flags = DT_NOPREFIX | DT_TOP;
    flags |= style.wordWrap ? DT_WORDBREAK : DT_SINGLELINE;
    switch (style.align) {
    case TextHorizontalAlign::Center:
        flags |= DT_CENTER;
        break;
    case TextHorizontalAlign::Right:
        flags |= DT_RIGHT;
        break;
    case TextHorizontalAlign::Left:
    default:
        flags |= DT_LEFT;
        break;
    }
    return flags;
}

HFONT createFont(const TextTextureStyle& style) {
    return CreateFontW(
        -static_cast<int>(std::max<std::uint32_t>(1, style.fontPixelHeight)),
        0,
        0,
        0,
        style.bold ? FW_BOLD : FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        style.fontFamily.c_str());
}

} // namespace

bool createTextTexture(
    const SpriteRenderer& renderer,
    const std::wstring_view text,
    const std::uint32_t width,
    const std::uint32_t maxHeight,
    const TextTextureStyle& style,
    TextTextureResult& result) {
    result = {};
    if (width == 0 || maxHeight == 0 || text.empty()) {
        return false;
    }

    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) {
        return false;
    }

    HFONT font = createFont(style);
    if (!font) {
        DeleteDC(dc);
        return false;
    }

    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);

    std::wstring ownedText{text};
    RECT measure{0, 0, static_cast<LONG>(width), 0};
    DrawTextW(
        dc,
        ownedText.c_str(),
        static_cast<int>(ownedText.size()),
        &measure,
        drawFlags(style) | DT_CALCRECT);

    const std::uint32_t measured = static_cast<std::uint32_t>(std::max<LONG>(1, measure.bottom - measure.top));
    const std::uint32_t textureHeight = std::clamp(measured + 2U, 1U, maxHeight);

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = static_cast<LONG>(width);
    bitmapInfo.bmiHeader.biHeight = -static_cast<LONG>(textureHeight);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* dibPixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &dibPixels, nullptr, 0);
    if (!bitmap || !dibPixels) {
        SelectObject(dc, oldFont);
        DeleteObject(font);
        DeleteDC(dc);
        return false;
    }

    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    const std::size_t byteCount = static_cast<std::size_t>(width) * textureHeight * 4U;
    std::memset(dibPixels, 0, byteCount);

    RECT drawRect{0, 0, static_cast<LONG>(width), static_cast<LONG>(textureHeight)};
    DrawTextW(
        dc,
        ownedText.c_str(),
        static_cast<int>(ownedText.size()),
        &drawRect,
        drawFlags(style));

    const auto* bgra = static_cast<const std::uint8_t*>(dibPixels);
    std::vector<std::uint8_t> rgba(byteCount, 0);
    for (std::size_t offset = 0; offset < byteCount; offset += 4U) {
        const std::uint8_t coverage = std::max({bgra[offset + 0], bgra[offset + 1], bgra[offset + 2]});
        rgba[offset + 0] = style.red;
        rgba[offset + 1] = style.green;
        rgba[offset + 2] = style.blue;
        rgba[offset + 3] = static_cast<std::uint8_t>(
            (static_cast<std::uint16_t>(coverage) * style.alpha) / 255U);
    }

    ID3D11Device* device = renderer.device();
    bool created = false;
    if (device) {
        D3D11_TEXTURE2D_DESC textureDesc{};
        textureDesc.Width = width;
        textureDesc.Height = textureHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = rgba.data();
        initialData.SysMemPitch = width * 4U;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> nativeTexture;
        if (SUCCEEDED(device->CreateTexture2D(&textureDesc, &initialData, nativeTexture.GetAddressOf()))) {
            D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
            viewDesc.Format = textureDesc.Format;
            viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            viewDesc.Texture2D.MipLevels = 1;
            created = SUCCEEDED(device->CreateShaderResourceView(
                nativeTexture.Get(),
                &viewDesc,
                result.texture.view.GetAddressOf()));
        }
    }

    if (created) {
        result.texture.width = width;
        result.texture.height = textureHeight;
        result.contentHeight = textureHeight;
    } else {
        result = {};
    }

    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    DeleteDC(dc);
    return created;
}

} // namespace eudoria
