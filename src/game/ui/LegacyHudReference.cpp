#include "game/ui/LegacyHudReference.h"

namespace eudoria::game::ui {

bool LegacyHudReference::initialize(SpriteRenderer& renderer, const std::filesystem::path& root) {
    bool loadedAny = false;
    for (auto& entry : entries_) {
        const auto path = root / entry.fileName;
        if (std::filesystem::exists(path) && renderer.loadTexture(path.wstring(), entry.texture)) {
            loadedAny = true;
        }
    }
    return loadedAny;
}

void LegacyHudReference::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    if (!enabled_) {
        return;
    }

    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();

    for (const auto& entry : entries_) {
        if (!entry.texture.valid()) {
            continue;
        }

        const auto root = viewport.mapRoot(entry.root, entry.anchor);
        const float x = root.x - entry.originX * scale;
        const float y = root.y - entry.originY * scale;
        renderer.draw(
            entry.texture,
            x,
            y,
            static_cast<float>(entry.texture.width) * scale,
            static_cast<float>(entry.texture.height) * scale);
    }
}

} // namespace eudoria::game::ui
