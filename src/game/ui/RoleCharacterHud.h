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
    struct TextVisual final {
        float x = 0.0F;
        float y = 0.0F;
        float width = 0.0F;
        TextTextureResult texture;
    };

    struct ReferenceBounds final {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        bool valid = false;
    };

    void addText(
        SpriteRenderer& renderer,
        const wchar_t* text,
        float x,
        float y,
        float width,
        std::uint32_t fontSize = 10,
        TextHorizontalAlign align = TextHorizontalAlign::Left);

    void loadReferenceBounds(const std::filesystem::path& runtimeRoot);

    SpriteTexture reference_;
    ReferenceBounds referenceBounds_;
    std::vector<TextVisual> texts_;
};

} // namespace eudoria::game::ui
