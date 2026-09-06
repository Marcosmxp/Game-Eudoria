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

    // UI fidelity rule for PlayerFullInfoUIMC/symbol1998:
    // use the original FFDec-rendered symbol as the static visual source of
    // truth. Decomposing the symbol into independently cropped PNGs changed
    // local origins and caused the missing panels/slots seen in testing.
    SpriteTexture reference_;
    float referenceX_ = -332.75F;
    float referenceY_ = -230.75F;
    float referenceWidth_ = 0.0F;
    float referenceHeight_ = 0.0F;
};

} // namespace eudoria::game::ui
