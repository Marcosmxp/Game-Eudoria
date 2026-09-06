#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/render/TextRasterizer.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace eudoria::game::ui {

class RoleCharacterHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/role_window/character");

    void render(SpriteRenderer& renderer, float rootX, float rootY) const;

private:
    enum class LayerKind : std::uint8_t {
        Sprite,
        Text,
    };

    struct LayerItem final {
        LayerKind kind = LayerKind::Sprite;
        int depth = 0;
        float x = 0.0F;
        float y = 0.0F;
        float width = 0.0F;
        float height = 0.0F;
        SpriteTexture sprite;
        TextTextureResult text;
    };

    bool loadVisualManifest(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot);

    bool loadTextManifest(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot);

    void addSyntheticText(
        SpriteRenderer& renderer,
        int depth,
        const wchar_t* text,
        float x,
        float y,
        float width,
        float height,
        std::uint32_t fontSize,
        TextHorizontalAlign align = TextHorizontalAlign::Center,
        std::uint8_t red = 255,
        std::uint8_t green = 255,
        std::uint8_t blue = 255);

    std::vector<LayerItem> layers_;
};

} // namespace eudoria::game::ui
