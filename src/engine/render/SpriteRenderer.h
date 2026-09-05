#pragma once

#include <cstdint>
#include <d3d11.h>
#include <string>
#include <wincodec.h>
#include <wrl/client.h>

namespace eudoria {

struct SpriteTexture final {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    [[nodiscard]] bool valid() const noexcept {
        return view != nullptr && width > 0 && height > 0;
    }
};

struct SpriteSourceRect final {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

class SpriteRenderer final {
public:
    SpriteRenderer() = default;
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    bool loadTexture(const std::wstring& path, SpriteTexture& texture) const;
    void begin(std::uint32_t width, std::uint32_t height);
    void draw(const SpriteTexture& texture, float x, float y, float width, float height, float alpha = 1.0F);
    void drawRegion(
        const SpriteTexture& texture,
        const SpriteSourceRect& source,
        float x,
        float y,
        float width,
        float height,
        float alpha = 1.0F);
    void shutdown();

    [[nodiscard]] ID3D11Device* device() const noexcept { return device_; }

private:
    struct Vertex final {
        float x;
        float y;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    };

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;
    std::uint32_t viewportWidth_ = 0;
    std::uint32_t viewportHeight_ = 0;
    bool ownsComInitialization_ = false;
};

} // namespace eudoria
