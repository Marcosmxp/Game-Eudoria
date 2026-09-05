#include "engine/render/D3D11Renderer.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iterator>

namespace eudoria {

bool D3D11Renderer::initialize(void* windowHandle, const std::uint32_t width, const std::uint32_t height) {
    width_ = width;
    height_ = height;

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = static_cast<HWND>(windowHandle);
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    constexpr D3D_FEATURE_LEVEL requestedLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL selectedLevel{};
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        requestedLevels,
        static_cast<UINT>(std::size(requestedLevels)),
        D3D11_SDK_VERSION,
        &swapChainDesc,
        swapChain_.GetAddressOf(),
        device_.GetAddressOf(),
        &selectedLevel,
        context_.GetAddressOf());

    if (result == E_INVALIDARG) {
        constexpr D3D_FEATURE_LEVEL fallbackLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            fallbackLevels,
            static_cast<UINT>(std::size(fallbackLevels)),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            swapChain_.GetAddressOf(),
            device_.GetAddressOf(),
            &selectedLevel,
            context_.GetAddressOf());
    }

    if (FAILED(result) || !createRenderTarget() || !spriteRenderer_.initialize(device_.Get(), context_.Get())) {
        shutdown();
        return false;
    }

    return true;
}

bool D3D11Renderer::createRenderTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    if (!swapChain_ || FAILED(swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())))) {
        return false;
    }

    return SUCCEEDED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTarget_.GetAddressOf()));
}

void D3D11Renderer::releaseRenderTarget() {
    if (context_) {
        context_->OMSetRenderTargets(0, nullptr, nullptr);
    }
    renderTarget_.Reset();
}

void D3D11Renderer::resize(const std::uint32_t width, const std::uint32_t height) {
    if (!swapChain_ || width == 0 || height == 0 || (width == width_ && height == height_)) {
        return;
    }

    width_ = width;
    height_ = height;
    releaseRenderTarget();

    if (SUCCEEDED(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
        createRenderTarget();
    }
}

void D3D11Renderer::beginFrame() {
    if (!context_ || !renderTarget_) {
        return;
    }

    constexpr float clearColor[4] = {0.025F, 0.03F, 0.035F, 1.0F};
    context_->OMSetRenderTargets(1, renderTarget_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(renderTarget_.Get(), clearColor);
    spriteRenderer_.begin(width_, height_);
}

void D3D11Renderer::endFrame() {
    if (swapChain_) {
        swapChain_->Present(1, 0);
    }
}

void D3D11Renderer::shutdown() {
    spriteRenderer_.shutdown();
    releaseRenderTarget();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    width_ = 0;
    height_ = 0;
}

} // namespace eudoria
