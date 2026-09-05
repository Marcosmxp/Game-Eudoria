#include "game/ui/SmallMapHud.h"

#include <algorithm>

namespace eudoria::game::ui {

bool SmallMapHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot,
    const std::filesystem::path& minimapRoot) {
    bool loadedAny = false;

    const auto load = [&](const std::filesystem::path& relative, SpriteTexture& target) {
        const bool loaded = renderer.loadTexture((runtimeRoot / relative).wstring(), target);
        loadedAny = loadedAny || loaded;
        return loaded;
    };

    load(L"base.png", base_);
    load(L"player_center.png", playerCenter_);
    load(L"zoom_out/up.png", zoomOut_);
    load(L"zoom_in/up.png", zoomIn_);
    load(L"online_bonus.png", onlineBonus_);
    load(L"map/up.png", mapButton_);
    load(L"remote_display.png", remoteDisplay_);
    load(L"world_map/up.png", worldMapButton_);
    load(L"shop/up.png", shop_);
    load(L"days_prompt/up.png", daysPrompt_);
    load(L"ranking/up.png", ranking_);
    load(L"day_bonus/up.png", dayBonus_);
    load(L"skill_effect.png", skillEffect_);
    load(L"drg_lottery/up.png", drgLottery_);
    load(L"misc_1694.png", misc1694_);
    load(L"result/up.png", result_);
    load(L"total_icon.png", totalIcon_);
    load(L"collapse/up.png", collapse_);

    loadMinimap(renderer, minimapRoot);
    return loadedAny;
}

bool SmallMapHud::loadMinimap(SpriteRenderer& renderer, const std::filesystem::path& minimapRoot) {
    minimap_ = {};
    if (!std::filesystem::exists(minimapRoot)) {
        return false;
    }

    const auto current = minimapRoot / L"current.jpg";
    if (std::filesystem::exists(current) && renderer.loadTexture(current.wstring(), minimap_)) {
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(minimapRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto extension = entry.path().extension().wstring();
        if (extension != L".jpg" && extension != L".jpeg" && extension != L".png") {
            continue;
        }
        if (renderer.loadTexture(entry.path().wstring(), minimap_)) {
            return true;
        }
    }

    return false;
}

void SmallMapHud::drawPlaced(
    SpriteRenderer& renderer,
    const SpriteTexture& texture,
    const eudoria::ui::Point& root,
    const eudoria::ui::Point& placement,
    const eudoria::ui::Point& rasterOffset,
    const float scale) {
    if (!texture.valid()) {
        return;
    }

    renderer.draw(
        texture,
        root.x + ((placement.x + rasterOffset.x) * scale),
        root.y + ((placement.y + rasterOffset.y) * scale),
        static_cast<float>(texture.width) * scale,
        static_cast<float>(texture.height) * scale);
}

void SmallMapHud::render(
    SpriteRenderer& renderer,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight) const {
    const eudoria::ui::LegacyViewport viewport{
        static_cast<float>(viewportWidth),
        static_cast<float>(viewportHeight),
    };
    const float scale = viewport.scale();
    const auto root = viewport.mapRoot(kRoot, kAnchor);

    // symbol1825 depth 1: background character1632.
    // shapes/1632.png is the real payload raster with local bounds [-181,0..192].
    drawPlaced(renderer, base_, root, {0.0F, 0.0F}, {-181.0F, 0.0F}, scale);

    // depth 2 mapRootPoint: the actual p<mapId>.jpg minimap belongs here.
    if (minimap_.valid()) {
        renderer.draw(
            minimap_,
            root.x + (kViewportX * scale),
            root.y + (kViewportY * scale),
            kViewportWidth * scale,
            kViewportHeight * scale);
    }

    // depth 4 playerCenter, character1634. Its recursive SWF bounds are -3..3.
    drawPlaced(renderer, playerCenter_, root, {-78.65F, 97.35F}, {-3.0F, -3.0F}, scale);

    // Direct SmallMap children in the same display-list order recovered from
    // assets.swf. Raster offsets were recovered by matching each independent
    // exported character back to its symbol1825 placement; screenshots are not
    // used as geometry input.
    drawPlaced(renderer, zoomOut_, root, {-41.10F, 156.70F}, {-30.90F, -17.20F}, scale);
    drawPlaced(renderer, zoomIn_, root, {-23.10F, 156.70F}, {-30.90F, -17.20F}, scale);
    drawPlaced(renderer, onlineBonus_, root, {-216.70F, 163.00F}, {-35.30F, -35.50F}, scale);
    drawPlaced(renderer, mapButton_, root, {-129.05F, 178.70F}, {-14.95F, -16.20F}, scale);
    drawPlaced(renderer, remoteDisplay_, root, {-45.15F, 178.70F}, {-49.85F, -16.20F}, scale);
    drawPlaced(renderer, worldMapButton_, root, {-101.65F, 178.70F}, {-27.35F, -16.20F}, scale);

    // cmdSite (1663) is intentionally omitted because SmallMapUI constructor
    // explicitly sets cmdSite.visible = false.
    drawPlaced(renderer, shop_, root, {-170.50F, 162.35F}, {-19.50F, -21.85F}, scale);
    drawPlaced(renderer, daysPrompt_, root, {-167.00F, 125.35F}, {-16.00F, -17.85F}, scale);
    drawPlaced(renderer, ranking_, root, {-167.00F, 93.35F}, {-16.00F, -17.85F}, scale);
    drawPlaced(renderer, dayBonus_, root, {-167.00F, 61.35F}, {-16.00F, -17.85F}, scale);
    drawPlaced(renderer, skillEffect_, root, {-74.10F, 178.70F}, {-49.90F, -16.20F}, scale);

    // cmdDrgLottery and cmdWheelAward both use character1691 at the same
    // placement in this payload. Their configured visibility resolves to a
    // single visible raster, so drawing it once is pixel-equivalent.
    drawPlaced(renderer, drgLottery_, root, {-168.05F, 25.50F}, {-60.95F, -33.00F}, scale);
    drawPlaced(renderer, misc1694_, root, {-17.10F, 178.70F}, {-13.90F, -15.20F}, scale);
    drawPlaced(renderer, result_, root, {-197.10F, 16.00F}, {-39.90F, -17.50F}, scale);

    // depth 54: totalIcon character1815. The sprite starts expanded in
    // SmallMapUI (mg = true). Its own FFDec raster starts at (-746.3, 0.55)
    // relative to the character origin; keeping it independent restores the
    // top icon panel without forcing the entire SmallMap composite onscreen.
    drawPlaced(renderer, totalIcon_, root, {-229.70F, 47.95F}, {-746.30F, 0.55F}, scale);

    // depth 157: up/collapse control. SmallMapUI starts expanded, so 'up' is
    // visible and 'down' is hidden until the interaction is migrated.
    drawPlaced(renderer, collapse_, root, {-219.65F, 45.95F}, {-7.35F, -10.45F}, scale);
}

} // namespace eudoria::game::ui
