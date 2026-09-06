#include "game/ui/RoleCharacterHud.h"

#include <fstream>
#include <string>

namespace eudoria::game::ui {
namespace {

// FFDec exports DefineSprite_1998 on a 563x723 canvas that intentionally keeps
// transparent padding. The canvas origin is not the visible equipment panel
// origin. We recover the exact local canvas origin from the payload itself:
//
// shape1884 local bounds:      x=-186.75..50.75, y=-164.75..74.75
// shape1884 placement:         x=-25, y=11
// shape1884 in symbol1998 PNG: pixel (121,77)
//
// Therefore:
//   rasterX = (-25 - 186.75) - 121 = -332.75
//   rasterY = ( 11 - 164.75) -  77 = -230.75
//
// The supplied screenshot was used only to validate the final appearance; this
// coordinate is derived from the SWF/raster payload, not from screenshot layout.
constexpr float kReferenceCanvasX = -332.75F;
constexpr float kReferenceCanvasY = -230.75F;

} // namespace

bool RoleCharacterHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    reference_ = {};
    referenceX_ = kReferenceCanvasX;
    referenceY_ = kReferenceCanvasY;
    referenceWidth_ = 0.0F;
    referenceHeight_ = 0.0F;

    if (!renderer.loadTexture((runtimeRoot / L"reference.png").wstring(), reference_)) {
        return false;
    }

    // Keep the original FFDec canvas at native pixel size. Its visible alpha
    // area is already the exact Flash composition of panels, slots, controls,
    // ornaments and static icons. No per-element rescaling occurs here.
    referenceWidth_ = static_cast<float>(reference_.width);
    referenceHeight_ = static_cast<float>(reference_.height);

    // Extraction writes the same calibrated origin. The constants remain a safe
    // fallback so an optional diagnostic manifest can never break the UI again.
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

        std::string header;
        std::string line;
        if (!std::getline(stream, header) || !std::getline(stream, line)) {
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
        referenceX_ = kReferenceCanvasX;
        referenceY_ = kReferenceCanvasY;
        return false;
    }
}

} // namespace eudoria::game::ui
