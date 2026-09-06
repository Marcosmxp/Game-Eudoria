#include "game/ui/RoleCharacterHud.h"

#include <fstream>
#include <string>

namespace eudoria::game::ui {

bool RoleCharacterHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    reference_ = {};
    referenceX_ = 0.0F;
    referenceY_ = 0.0F;
    referenceWidth_ = 0.0F;
    referenceHeight_ = 0.0F;

    if (!renderer.loadTexture((runtimeRoot / L"reference.png").wstring(), reference_)) {
        return false;
    }

    // The FFDec raster is already a complete render of the original
    // PlayerFullInfoUIMC first frame. Use its actual pixel dimensions; only its
    // local origin needs to be recovered from reference_bounds.tsv.
    referenceWidth_ = static_cast<float>(reference_.width);
    referenceHeight_ = static_cast<float>(reference_.height);
    loadReferenceBounds(runtimeRoot);
    return reference_.valid();
}

void RoleCharacterHud::render(
    SpriteRenderer& renderer,
    const float rootX,
    const float rootY) const {
    if (!reference_.valid()) {
        return;
    }

    renderer.draw(
        reference_,
        rootX + referenceX_,
        rootY + referenceY_,
        referenceWidth_,
        referenceHeight_);
}

bool RoleCharacterHud::loadReferenceBounds(
    const std::filesystem::path& runtimeRoot) noexcept {
    try {
        std::ifstream stream(runtimeRoot / L"reference_bounds.tsv", std::ios::binary);
        if (!stream) {
            return false;
        }

        std::string line;
        if (!std::getline(stream, line)) {
            return false;
        }
        if (!std::getline(stream, line)) {
            return false;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::size_t first = line.find('\t');
        if (first == std::string::npos) {
            return false;
        }
        const std::size_t second = line.find('\t', first + 1);
        if (second == std::string::npos) {
            return false;
        }

        referenceX_ = std::stof(line.substr(0, first));
        referenceY_ = std::stof(line.substr(first + 1, second - first - 1));
        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace eudoria::game::ui
