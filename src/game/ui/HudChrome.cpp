#include "game/ui/HudChrome.h"

namespace eudoria::game::ui {

bool HudChrome::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& referenceRoot) {
    bool loadedAny = false;
    for (auto& panel : panels_) {
        if (renderer.loadTexture((referenceRoot / panel.fileName).wstring(), panel.texture)) {
            loadedAny = true;
        }
    }
    return loadedAny;
}

void HudChrome::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();

    for (const auto& panel : panels_) {
        if (!panel.texture.valid()) {
            continue;
        }
        const auto root = viewport.mapRoot(panel.root, panel.anchor);
        renderer.draw(
            panel.texture,
            root.x - (panel.originX * scale),
            root.y - (panel.originY * scale),
            static_cast<float>(panel.texture.width) * scale,
            static_cast<float>(panel.texture.height) * scale);
    }
}

} // namespace eudoria::game::ui
