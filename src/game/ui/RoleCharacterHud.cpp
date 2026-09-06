#include "game/ui/RoleCharacterHud.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

namespace eudoria::game::ui {
namespace {

TextTextureStyle roleTextStyle(
    const std::uint32_t size,
    const TextHorizontalAlign align) {
    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = size;
    style.red = 238;
    style.green = 238;
    style.blue = 220;
    style.align = align;
    style.wordWrap = false;
    return style;
}

struct ButtonLabel final {
    const wchar_t* text;
    float x;
    float y;
    float width;
};

constexpr ButtonLabel kButtonLabels[] = {
    {L"Arena Rewards", 98.55F, -225.5F, 114.0F},
    {L"Achievements", 82.5F, -201.0F, 130.0F},
    {L"Title", -24.5F, -201.0F, 60.0F},
    {L"Nobility", 92.3F, -36.0F, 120.0F},
    {L"Synthesize", 28.55F, 67.5F, 75.0F},
    {L"Gear Evolve", -130.65F, -2.5F, 80.0F},
    {L"Auto", -207.5F, 191.0F, 75.0F},
    {L"OK", -128.5F, 191.0F, 75.0F},
};

} // namespace

bool RoleCharacterHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    // For this window the complete PlayerFullInfoUIMC first-frame raster is a
    // valid visual baseline: unlike SmallMap/ControlBar it does not contain an
    // unrelated expandable HUD panel that must be hidden at runtime. Using the
    // exact symbol1998 raster removes the accumulated geometric error introduced
    // by rebuilding dozens of decorative children independently.
    const bool referenceLoaded = renderer.loadTexture(
        (runtimeRoot / L"reference.png").wstring(),
        reference_);
    loadReferenceBounds(runtimeRoot);

    texts_.clear();

    // Timeline text fields are populated by the legacy controller. We keep only
    // those strings/neutral values as native overlays; all chrome, slots,
    // separators, special feature artwork and button skins come from symbol1998.
    addText(renderer, L"Arena Score:", -208.0F, -220.0F, 79.0F, 9);
    addText(renderer, L"0", -129.0F, -220.0F, 50.0F, 9);
    addText(renderer, L"Current Tier:", -51.0F, -220.0F, 79.0F, 9);
    addText(renderer, L"-", 28.0F, -220.0F, 48.0F, 9);

    addText(renderer, L"Title:", -202.0F, -198.0F, 48.0F);
    addText(renderer, L"-", -153.0F, -198.0F, 124.0F);
    addText(renderer, L"EXP:", -202.0F, -176.0F, 48.0F);
    addText(renderer, L"0 / 0", -152.5F, -176.0F, 146.0F);

    addText(renderer, L"Level:", 42.0F, -176.0F, 86.0F);
    addText(renderer, L"1", 129.0F, -176.0F, 80.0F);
    addText(renderer, L"Class:", 42.0F, -146.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -146.0F, 79.0F);
    addText(renderer, L"Plane:", 42.0F, -116.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -116.0F, 79.0F);
    addText(renderer, L"Rank:", 42.0F, -85.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -85.0F, 79.0F);
    addText(renderer, L"Honor:", 42.0F, -55.0F, 86.0F);
    addText(renderer, L"0", 135.0F, -55.0F, 75.0F);
    addText(renderer, L"Guild:", 42.0F, -1.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -1.0F, 79.0F);
    addText(renderer, L"PK Lvl:", 42.0F, 34.0F, 86.0F);
    addText(renderer, L"0", 129.0F, 34.0F, 41.0F);

    // Equipment slot captions come from txt/idc.json -> RoleUI.
    addText(renderer, L"Helm", -205.5F, -138.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Cape", -154.5F, -138.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Banner", -66.5F, -138.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Sperion", -24.0F, -138.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Armor", -205.5F, -100.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Artefact", -154.0F, -100.0F, 39.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Insig", -24.0F, -100.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Gloves", -205.5F, -62.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Wings", -24.0F, -62.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Boots", -205.5F, -24.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Neck", -24.0F, -24.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Offhand", -205.5F, 14.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Ring", -24.0F, 14.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Weap", -205.5F, 52.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Hat", -130.5F, 45.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Coat", -88.5F, 45.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Mount", -24.0F, 52.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Talisman", 44.5F, 29.0F, 40.0F, 8, TextHorizontalAlign::Center);

    // Costume label is controller text; the checkbox/skin itself belongs to the
    // exact reference raster. Player appearance switching is deferred.
    addText(renderer, L"Show Costume", -157.0F, 18.0F, 96.0F, 8, TextHorizontalAlign::Center);

    // Primary attributes.
    const struct Row { const wchar_t* name; float y; } primary[] = {
        {L"Strength:",95.0F}, {L"Agility:",114.0F},
        {L"Intellect:",133.0F}, {L"Endurance:",152.0F},
    };
    for (const auto& row : primary) {
        addText(renderer, row.name, -215.5F, row.y, 71.0F);
        addText(renderer, L"0", -144.5F, row.y, 43.0F);
    }
    addText(renderer, L"Points:", -214.5F, 171.0F, 70.0F);
    addText(renderer, L"0", -145.0F, 171.0F, 45.0F);

    // Resources/combat values.
    const struct StatPair { const wchar_t* name; float labelX; float valueX; float y; } stats[] = {
        {L"HP:",-40.0F,15.5F,98.0F}, {L"MP:",-40.0F,15.5F,114.0F},
        {L"Crystal Essence:",-40.0F,15.5F,130.0F},
        {L"PATK:",-39.7F,15.0F,146.0F}, {L"MATK:",-39.7F,15.0F,162.0F},
        {L"Hit:",-39.7F,15.0F,178.0F}, {L"Heal:",-39.7F,15.0F,194.0F},
        {L"Charm:",72.0F,137.0F,114.0F}, {L"Prestige:",82.0F,137.0F,130.0F},
        {L"PDEF:",81.0F,137.0F,146.0F}, {L"MDEF:",81.0F,137.0F,162.0F},
        {L"Dodge:",81.0F,137.0F,178.0F}, {L"Crit:",81.0F,137.0F,194.0F},
    };
    for (const auto& stat : stats) {
        addText(renderer, stat.name, stat.labelX, stat.y, 65.0F, 9);
        addText(renderer, L"0", stat.valueX, stat.y, 65.0F, 9);
    }

    const struct Bonus { const wchar_t* name; float labelX; float valueX; float y; } bonus[] = {
        {L"Atk Spd:",-205.0F,-124.0F,224.0F}, {L"Cast Spd:",-205.0F,-124.0F,240.0F},
        {L"Move Spd:",-204.5F,-123.5F,256.0F}, {L"HP Regen:",-63.5F,18.5F,224.5F},
        {L"MP Regen:",-63.5F,18.5F,240.5F}, {L"Luck:",-63.5F,18.5F,256.5F},
        {L"Luck Def:",-63.5F,18.5F,272.5F}, {L"Crit Dmg:",71.5F,153.5F,224.5F},
        {L"Crit Def:",71.5F,153.5F,240.5F}, {L"AP:",71.5F,153.5F,256.5F},
        {L"AP Def:",71.5F,153.5F,272.5F},
    };
    for (const auto& stat : bonus) {
        addText(renderer, stat.name, stat.labelX, stat.y, 82.0F, 9);
        addText(renderer, L"0", stat.valueX, stat.y, 52.0F, 9);
    }

    addText(renderer, L"LOT:", -179.0F, 302.0F, 43.0F, 9);
    addText(renderer, L"0", -136.5F, 302.0F, 82.0F, 9);
    addText(renderer, L"LOA:", -55.5F, 302.0F, 43.0F, 9);
    addText(renderer, L"0", -13.5F, 302.0F, 82.0F, 9);
    addText(renderer, L"LOH:", 67.5F, 302.0F, 43.0F, 9);
    addText(renderer, L"0", 109.5F, 302.0F, 82.0F, 9);

    for (const auto& button : kButtonLabels) {
        addText(renderer, button.text, button.x, button.y, button.width, 9, TextHorizontalAlign::Center);
    }

    return referenceLoaded && referenceBounds_.valid;
}

void RoleCharacterHud::render(
    SpriteRenderer& renderer,
    const float rootX,
    const float rootY) const {
    if (!reference_.valid() || !referenceBounds_.valid) {
        return;
    }

    // FFDec renders symbol1998 at native Flash pixels. The root bounds written
    // from assets.swf give its exact local top-left. Do not stretch or rebuild
    // the static chrome: this is the fidelity baseline for the Character panel.
    renderer.draw(
        reference_,
        rootX + referenceBounds_.left,
        rootY + referenceBounds_.top,
        static_cast<float>(reference_.width),
        static_cast<float>(reference_.height));

    for (const auto& text : texts_) {
        if (!text.texture.texture.valid()) {
            continue;
        }
        renderer.draw(
            text.texture.texture,
            rootX + text.x,
            rootY + text.y,
            text.width,
            static_cast<float>(text.texture.texture.height));
    }
}

void RoleCharacterHud::addText(
    SpriteRenderer& renderer,
    const wchar_t* text,
    const float x,
    const float y,
    const float width,
    const std::uint32_t fontSize,
    const TextHorizontalAlign align) {
    TextVisual visual;
    visual.x = x;
    visual.y = y;
    visual.width = width;
    createTextTexture(
        renderer,
        text,
        std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(width)),
        24,
        roleTextStyle(fontSize, align),
        visual.texture);
    texts_.push_back(std::move(visual));
}

void RoleCharacterHud::loadReferenceBounds(
    const std::filesystem::path& runtimeRoot) {
    referenceBounds_ = {};

    std::ifstream stream(runtimeRoot / L"reference_bounds.tsv", std::ios::binary);
    if (!stream) {
        return;
    }

    std::string header;
    std::string values;
    if (!std::getline(stream, header) || !std::getline(stream, values)) {
        return;
    }
    if (!values.empty() && values.back() == '\r') {
        values.pop_back();
    }

    std::vector<std::string> fields;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = values.find('\t', begin);
        if (end == std::string::npos) {
            fields.emplace_back(values.substr(begin));
            break;
        }
        fields.emplace_back(values.substr(begin, end - begin));
        begin = end + 1;
    }

    if (fields.size() < 4) {
        return;
    }

    try {
        referenceBounds_.left = std::stof(fields[0]);
        referenceBounds_.top = std::stof(fields[1]);
        referenceBounds_.right = std::stof(fields[2]);
        referenceBounds_.bottom = std::stof(fields[3]);
        referenceBounds_.valid = referenceBounds_.right > referenceBounds_.left &&
            referenceBounds_.bottom > referenceBounds_.top;
    }
    catch (...) {
        referenceBounds_ = {};
    }
}

} // namespace eudoria::game::ui
