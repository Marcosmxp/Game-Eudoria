#pragma once

#include "engine/render/SpriteRenderer.h"

#include <filesystem>

namespace eudoria::game::ui {

class RoleCharacterHud final {
public:
    bool initialize(
        SpriteRenderer& renderer,
        const std::filesystem::path& runtimeRoot = "legacy_assets/runtime/ui/role_window/character");

    void render(SpriteRenderer& renderer, float rootX, float rootY) const;

private:
    bool loadReferenceBounds(const std::filesystem::path& runtimeRoot) noexcept;

    // During the UI fidelity pass the full first-frame raster of
    // PlayerFullInfoUIMC/symbol1998 is the visual source of truth. It comes
    // directly from Crystal Saga.rar and therefore preserves every panel,
    // equipment slot, button chrome, separator, icon placeholder and ornament
    // exactly as Flash rendered it. Dynamic player data is intentionally kept
    // out until the static UI matches the original reference.
    SpriteTexture reference_;
    float referenceX_ = 0.0F;
    float referenceY_ = 0.0F;
    float referenceWidth_ = 0.0F;
    float referenceHeight_ = 0.0F;
};

} // namespace eudoria::game::ui
