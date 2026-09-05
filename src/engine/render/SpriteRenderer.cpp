#include "engine/render/SpriteRenderer.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <objbase.h>
#include <wincodec.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <vector>

namespace eudoria {
namespace {

constexpr char kVertexShaderSource[] = R"(
struct VSInput {
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
)";

constexpr char kPixelShaderSource[] = R"(
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

float4 main(PSInput input) : SV_TARGET {
    return texture0.Sample(sampler0, input.uv) * input.color;
}
)";

bool compileShader(
    const char* source,
    const char* target,
    Microsoft::WRL::ComPtr<ID3DBlob>& bytecode) {
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(
        source,
        std::strlen(source),
        nullptr,
        nullptr,
        nullptr,
        "main",
        target,
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        bytecode.GetAddressOf(),
        errors.GetAddressOf());
    return SUCCEEDED(result);
}

} // namespace

SpriteRenderer::~SpriteRenderer() {
    shutdown();
}

bool SpriteRenderer::initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    shutdown();

    if (!device || !context) {
        return false;
    }

    device_ = device;
    context_ = context;

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(comResult)) {
        ownsComInitialization_ = true;
    } else if (comResult != RPC_E_CHANGED_MODE) {
        shutdown();
        return false;
    }

    HRESULT result = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(wicFactory_.GetAddressOf()));
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> vertexBytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelBytecode;
    if (!compileShader(kVertexShaderSource, "vs_5_0", vertexBytecode) ||
        !compileShader(kPixelShaderSource, "ps_5_0", pixelBytecode)) {
        shutdown();
        return false;
    }

    result = device_->CreateVertexShader(
        vertexBytecode->GetBufferPointer(),
        vertexBytecode->GetBufferSize(),
        nullptr,
        vertexShader_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    result = device_->CreatePixelShader(
        pixelBytecode->GetBufferPointer(),
        pixelBytecode->GetBufferSize(),
        nullptr,
        pixelShader_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    constexpr D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, u), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, r), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    result = device_->CreateInputLayout(
        inputElements,
        static_cast<UINT>(std::size(inputElements)),
        vertexBytecode->GetBufferPointer(),
        vertexBytecode->GetBufferSize(),
        inputLayout_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * 6;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = device_->CreateBuffer(&vertexBufferDesc, nullptr, vertexBuffer_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = device_->CreateSamplerState(&samplerDesc, sampler_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    result = device_->CreateBlendState(&blendDesc, blendState_.GetAddressOf());
    if (FAILED(result)) {
        shutdown();
        return false;
    }

    return true;
}

bool SpriteRenderer::loadTexture(const std::wstring& path, SpriteTexture& texture) const {
    texture = {};
    if (!device_ || !wicFactory_) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT result = wicFactory_->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        decoder.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    result = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    result = frame->GetSize(&width, &height);
    if (FAILED(result) || width == 0 || height == 0) {
        return false;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    result = wicFactory_->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    result = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(result)) {
        return false;
    }

    const UINT stride = width * 4;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(stride) * height);
    result = converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data());
    if (FAILED(result)) {
        return false;
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = pixels.data();
    initialData.SysMemPitch = stride;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> nativeTexture;
    result = device_->CreateTexture2D(&textureDesc, &initialData, nativeTexture.GetAddressOf());
    if (FAILED(result)) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
    viewDesc.Format = textureDesc.Format;
    viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipLevels = 1;
    result = device_->CreateShaderResourceView(nativeTexture.Get(), &viewDesc, texture.view.GetAddressOf());
    if (FAILED(result)) {
        texture = {};
        return false;
    }

    texture.width = width;
    texture.height = height;
    return true;
}

void SpriteRenderer::begin(const std::uint32_t width, const std::uint32_t height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    if (!context_ || width == 0 || height == 0) {
        return;
    }

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;
    context_->RSSetViewports(1, &viewport);

    constexpr UINT stride = sizeof(Vertex);
    constexpr UINT offset = 0;
    ID3D11Buffer* buffer = vertexBuffer_.Get();
    context_->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(inputLayout_.Get());
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);

    ID3D11SamplerState* sampler = sampler_.Get();
    context_->PSSetSamplers(0, 1, &sampler);

    constexpr float blendFactor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    context_->OMSetBlendState(blendState_.Get(), blendFactor, 0xFFFFFFFFU);
}

void SpriteRenderer::draw(
    const SpriteTexture& texture,
    const float x,
    const float y,
    const float width,
    const float height,
    const float alpha) {
    if (!context_ || !texture.valid() || viewportWidth_ == 0 || viewportHeight_ == 0 || width <= 0.0F || height <= 0.0F) {
        return;
    }

    const float left = (x / static_cast<float>(viewportWidth_)) * 2.0F - 1.0F;
    const float right = ((x + width) / static_cast<float>(viewportWidth_)) * 2.0F - 1.0F;
    const float top = 1.0F - (y / static_cast<float>(viewportHeight_)) * 2.0F;
    const float bottom = 1.0F - ((y + height) / static_cast<float>(viewportHeight_)) * 2.0F;
    const float a = std::clamp(alpha, 0.0F, 1.0F);

    const std::array<Vertex, 6> vertices{{
        {left, top, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, a},
        {right, top, 1.0F, 0.0F, 1.0F, 1.0F, 1.0F, a},
        {right, bottom, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, a},
        {left, top, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, a},
        {right, bottom, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, a},
        {left, bottom, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F, a},
    }};

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return;
    }
    std::memcpy(mapped.pData, vertices.data(), sizeof(vertices));
    context_->Unmap(vertexBuffer_.Get(), 0);

    ID3D11ShaderResourceView* view = texture.view.Get();
    context_->PSSetShaderResources(0, 1, &view);
    context_->Draw(6, 0);
}

void SpriteRenderer::shutdown() {
    if (context_) {
        ID3D11ShaderResourceView* nullView = nullptr;
        context_->PSSetShaderResources(0, 1, &nullView);
    }

    blendState_.Reset();
    sampler_.Reset();
    vertexBuffer_.Reset();
    inputLayout_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    wicFactory_.Reset();
    context_ = nullptr;
    device_ = nullptr;
    viewportWidth_ = 0;
    viewportHeight_ = 0;

    if (ownsComInitialization_) {
        CoUninitialize();
        ownsComInitialization_ = false;
    }
}

} // namespace eudoria
