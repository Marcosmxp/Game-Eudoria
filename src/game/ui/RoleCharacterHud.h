#pragma once

#include "engine/render/SpriteRenderer.h"
#include "engine/render/TextRasterizer.h"

#include <array>
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
    struct TexturePlacement final {
        float x = 0.0F;
        float y = 0.0F;
        float scaleX = 1.0F;
        float scaleY = 1.0F;
    };

    struct TextVisual final {
        float x = 0.0F;
        float y = 0.0F;
        float width = 0.0F;
        TextTextureResult texture;
    };

    static constexpr std::array<TexturePlacement, 18> kEquipmentSlots{{
        {-17.5F,-146.0F,1.0F,1.0F}, {-17.5F,-32.0F,1.0F,1.0F},
        {-17.5F,6.0F,1.0F,1.0F}, {-17.5F,-70.0F,1.0F,1.0F},
        {-17.5F,-108.0F,1.0F,1.0F}, {-17.5F,44.0F,1.0F,1.0F},
        {-128.0F,44.0F,1.0F,1.0F}, {-86.0F,44.0F,1.0F,1.0F},
        {-199.0F,-32.0F,1.0F,1.0F}, {-199.0F,-70.0F,1.0F,1.0F},
        {-199.0F,-108.0F,1.0F,1.0F}, {-199.0F,6.0F,1.0F,1.0F},
        {-199.0F,44.0F,1.0F,1.0F}, {-199.0F,-146.0F,1.0F,1.0F},
        {-60.0F,-146.0F,1.0F,1.0F}, {-152.0F,-146.0F,1.0F,1.0F},
        {47.0F,28.35F,1.0F,1.0F}, {-152.0F,-108.0F,1.0F,1.0F},
    }};

    static constexpr std::array<TexturePlacement, 4> kPanels{{
        {-130.0F,154.0F,3.413543701171875F,2.6248321533203125F},
        {82.0F,154.0F,5.353424072265625F,2.625335693359375F},
        {0.0F,256.0F,8.753082275390625F,1.5206298828125F},
        {0.0F,309.95F,8.753082275390625F,0.6249237060546875F},
    }};

    static constexpr std::array<TexturePlacement, 5> kValueBacks{{
        {168.8F,-137.75F,2.02447509765625F,0.4761962890625F},
        {168.8F,-107.75F,2.02447509765625F,0.4761962890625F},
        {168.8F,-76.75F,2.02447509765625F,0.4761962890625F},
        {168.8F,14.15F,2.02447509765625F,0.80426025390625F},
        {168.8F,42.25F,2.02447509765625F,0.4761962890625F},
    }};

    static constexpr std::array<TexturePlacement, 4> kAttrAdd{{
        {-77.0F,103.4F,1.0F,1.0F}, {-77.0F,122.5F,1.0F,1.0F},
        {-77.0F,141.0F,1.0F,1.0F}, {-77.0F,160.5F,1.0F,1.0F},
    }};
    static constexpr std::array<TexturePlacement, 4> kAttrRemove{{
        {-61.4F,103.4F,1.0F,1.0F}, {-61.4F,122.5F,1.0F,1.0F},
        {-61.4F,141.0F,1.0F,1.0F}, {-61.4F,160.5F,1.0F,1.0F},
    }};
    static constexpr std::array<TexturePlacement, 4> kAttrAddAll{{
        {-93.0F,103.4F,1.0F,1.0F}, {-93.0F,122.5F,1.0F,1.0F},
        {-93.0F,141.0F,1.0F,1.0F}, {-93.0F,160.5F,1.0F,1.0F},
    }};

    void addText(
        SpriteRenderer& renderer,
        const wchar_t* text,
        float x,
        float y,
        float width,
        std::uint32_t fontSize = 10,
        TextHorizontalAlign align = TextHorizontalAlign::Left);

    void drawSpriteAtBounds(
        SpriteRenderer& renderer,
        const SpriteTexture& texture,
        float rootX,
        float rootY,
        const TexturePlacement& placement,
        float boundsLeft,
        float boundsTop,
        float boundsWidth,
        float boundsHeight) const;

    SpriteTexture equipmentPanel_;
    SpriteTexture equipmentSlot_;
    SpriteTexture panel_;
    SpriteTexture valueBack_;
    SpriteTexture progress100_;
    SpriteTexture attrAdd_;
    SpriteTexture attrRemove_;
    SpriteTexture attrAddAll_;
    SpriteTexture mainButton_;

    std::vector<TextVisual> texts_;
};

} // namespace eudoria::game::ui
