#pragma once

#include <cstdint>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace eudoria {

class D3D11Renderer final {
public:
    D3D11Renderer() = default;
    ~D3D11Renderer() = default;

    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    bool initialize(void* windowHandle, std::uint32_t width, std::uint32_t height);
    void resize(std::uint32_t width, std::uint32_t height);
    void render();
    void shutdown();

private:
    bool createRenderTarget();
    void releaseRenderTarget();

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

} // namespace eudoria
