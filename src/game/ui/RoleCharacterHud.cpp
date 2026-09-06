#include "game/ui/RoleCharacterHud.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace eudoria::game::ui {
namespace {

struct FieldText final {
    std::string_view name;
    std::wstring_view text;
};

constexpr std::array kFieldTexts{
    FieldText{"txtDeyEquipSoul", L"Sperion"},
    FieldText{"txtDeyEquipAmulet", L"Neck"},
    FieldText{"txtDeyEquipRing", L"Ring"},
    FieldText{"txtDeyEquipWing", L"Wings"},
    FieldText{"txtDeyEquipMedal", L"Insig"},
    FieldText{"txtDeyEquipRide", L"Mount"},
    FieldText{"txtDeyEquipHat", L"Helm"},
    FieldText{"txtDeyEquipWeapon", L"Weap"},
    FieldText{"txtDeyEquipShield", L"Offhand"},
    FieldText{"txtDeyEquipClothes", L"Armor"},
    FieldText{"txtDeyEquipGlove", L"Gloves"},
    FieldText{"txtDeyEquipShoes", L"Boots"},
    FieldText{"txtDeyEquipFashionHat", L"Hat"},
    FieldText{"txtDeyEquipFashionClothes", L"Coat"},
    FieldText{"txtDeyEquipBanner", L"Banner"},
    FieldText{"txtDeyEquipCloak", L"Cape"},
    FieldText{"txtDeyEquipTalisman", L"Talisman"},
    FieldText{"txtDeyEquipHoly", L"Artefact"},

    FieldText{"txtDeyStrength", L"Strength:"},
    FieldText{"txtDeyAgility", L"Agility:"},
    FieldText{"txtDeyIntellect", L"Intellect:"},
    FieldText{"txtDeyStamina", L"Endurance:"},
    FieldText{"txtStrength", L"0"},
    FieldText{"txtAgility", L"0"},
    FieldText{"txtIntellect", L"0"},
    FieldText{"txtStamina", L"0"},
    FieldText{"txtDeyAttrPoint", L"Points:"},
    FieldText{"txtAttrPoint", L"0"},

    FieldText{"txtDeyPhysicalAttack", L"PATK:"},
    FieldText{"txtDeyMagicAttack", L"MATK:"},
    FieldText{"txtDeyCure", L"Heal:"},
    FieldText{"txtDeyPhysicalDefense", L"PDEF:"},
    FieldText{"txtDeyMagicDefense", L"MDEF:"},
    FieldText{"txtDeyAccuracy", L"Hit:"},
    FieldText{"txtDeyDodge", L"Dodge:"},
    FieldText{"txtDeyCrit", L"Crit:"},
    FieldText{"txtPhysicalAttack", L"0"},
    FieldText{"txtMagicAttack", L"0"},
    FieldText{"txtCure", L"0"},
    FieldText{"txtPhysicalDefense", L"0"},
    FieldText{"txtMagicDefense", L"0"},
    FieldText{"txtAccuracy", L"0"},
    FieldText{"txtDodge", L"0"},
    FieldText{"txtCrit", L"0"},

    FieldText{"txtDeyHp", L"HP:"},
    FieldText{"txtDeyMp", L"MP:"},
    FieldText{"txtDeySp", L"Crystal Essence:"},
    FieldText{"txtHp", L"0 / 0"},
    FieldText{"txtMp", L"0 / 0"},
    FieldText{"txtSp", L"0"},
    FieldText{"txtDeyCharmValue", L"Charm:"},
    FieldText{"txtCharmValue", L"0"},
    FieldText{"txtDeyCharmInt", L"Prestige:"},
    FieldText{"txtCharmInt", L"0"},

    FieldText{"txtLuck", L"0"},
    FieldText{"txtCritDamage", L"0"},
    FieldText{"txtPierceAttack", L"0"},
    FieldText{"txtHpHeal", L"0"},
    FieldText{"txtMpHeal", L"0"},
    FieldText{"txtAttackSpeed", L"0"},
    FieldText{"txtSingSpeed", L"0"},
    FieldText{"txtWalkSpeed", L"0"},
    FieldText{"txtLuckDefense", L"0"},
    FieldText{"txtCritDefense", L"0"},
    FieldText{"txtPierceDefense", L"0"},
    FieldText{"txtDeyLuck", L"Luck:"},
    FieldText{"txtDeyCritDamage", L"Crit Dmg:"},
    FieldText{"txtDeyPierceAttack", L"AP:"},
    FieldText{"txtDeyHpHeal", L"HP Regen:"},
    FieldText{"txtDeyMpHeal", L"MP Regen:"},
    FieldText{"txtDeyAttackSpeed", L"Atk Spd:"},
    FieldText{"txtDeySingSpeed", L"Cast Spd:"},
    FieldText{"txtDeyWalkSpeed", L"Move Spd:"},
    FieldText{"txtDeyLuckDefense", L"Luck Def:"},
    FieldText{"txtDeyCritDefense", L"Crit Def:"},
    FieldText{"txtDeyPierceDefense", L"AP Def:"},

    FieldText{"txtLOH", L"0(0.00%)"},
    FieldText{"txtLOA", L"0(0.00%)"},
    FieldText{"txtLOT", L"0(0.00%)"},
    FieldText{"txtDeyLOH", L"LOH:"},
    FieldText{"txtDeyLOA", L"LOA:"},
    FieldText{"txtDeyLOT", L"LOT:"},

    FieldText{"txtDeyJob", L"Class:"},
    FieldText{"txtDeyState", L"Plane:"},
    FieldText{"txtDeyRank", L"Rank:"},
    FieldText{"txtDeyPrestige", L"Honor:"},
    FieldText{"txtDeyGild", L"Guild:"},
    FieldText{"txtDeyPK", L"PK Lvl:"},
    FieldText{"txtJob", L"-"},
    FieldText{"txtState", L"-"},
    FieldText{"txtRank", L"-"},
    FieldText{"txtGild", L"-"},
    FieldText{"txtPK", L"0"},
    FieldText{"txtPrestige", L"0"},

    FieldText{"txtDeyFateSkill", L"Soul"},
    FieldText{"txtDeyLevel", L"Level:"},
    FieldText{"txtDeyPlayerTitle", L"Title:"},
    FieldText{"txtDeyExp", L"EXP:"},
    FieldText{"txtLevel", L"1"},
    FieldText{"txtExp", L"0/100"},
    FieldText{"txtPlayerTitle", L""},
    FieldText{"txtDeyMaxLevel", L""},
    FieldText{"txtMaxLevel", L"120"},
    FieldText{"txtDeyArena", L"Arena Score:"},
    FieldText{"txtArena", L"0"},
    FieldText{"txtDeyArenaPlace", L"Current Tier:"},
    FieldText{"txtArenaPlace", L"Tier 1"},
};

std::vector<std::string> splitTsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = line.find('\t', begin);
        if (end == std::string::npos) {
            fields.emplace_back(line.substr(begin));
            break;
        }
        fields.emplace_back(line.substr(begin, end - begin));
        begin = end + 1;
    }
    return fields;
}

std::wstring_view fieldText(const std::string_view name) noexcept {
    for (const auto& field : kFieldTexts) {
        if (field.name == name) {
            return field.text;
        }
    }
    return {};
}

TextHorizontalAlign flashAlign(const int value) noexcept {
    switch (value) {
    case 1: return TextHorizontalAlign::Right;
    case 2: return TextHorizontalAlign::Center;
    default: return TextHorizontalAlign::Left;
    }
}

bool hideWhenNoFreeAttributePoints(const std::string_view name) noexcept {
    return name == "cmdStrAddPoint" ||
        name == "cmdStrRemovePoint" ||
        name == "cmdAgiAddPoint" ||
        name == "cmdAgiRemovePoint" ||
        name == "cmdWigAddPoint" ||
        name == "cmdWisRemovePoint" ||
        name == "cmdVitAddPoint" ||
        name == "cmdVitRemovePoint" ||
        name == "cmdStrAddAllPoint" ||
        name == "cmdAgiAddAllPoint" ||
        name == "cmdWisAddAllPoint" ||
        name == "cmdVitAddAllPoint" ||
        name == "cmdPointOK" ||
        name == "cmdPointAuto";
}

const wchar_t* visualButtonLabel(const std::string_view name) noexcept {
    if (name == "cmdTalisman") return L"Synthesize";
    if (name == "cmdHonorPrompt") return L"Nobility";
    if (name == "cmdPlayerTitle") return L"Title";
    if (name == "cmdSuccess") return L"Achievements";
    if (name == "cmdArenaPlace") return L"Arena Rewards";
    if (name == "cmdEquKindGrow") return L"Gear Evolve";
    if (name == "chkShowFashion") return L"Show Costume";
    return nullptr;
}

} // namespace

bool RoleCharacterHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    layers_.clear();

    const bool visualsLoaded = loadVisualManifest(renderer, runtimeRoot);
    const bool textsLoaded = loadTextManifest(renderer, runtimeRoot);

    std::stable_sort(
        layers_.begin(),
        layers_.end(),
        [](const LayerItem& left, const LayerItem& right) {
            return left.depth < right.depth;
        });

    return visualsLoaded && textsLoaded;
}

void RoleCharacterHud::render(
    SpriteRenderer& renderer,
    const float rootX,
    const float rootY) const {
    for (const auto& layer : layers_) {
        if (layer.kind == LayerKind::Sprite) {
            if (!layer.sprite.valid()) {
                continue;
            }
            renderer.draw(
                layer.sprite,
                rootX + layer.x,
                rootY + layer.y,
                layer.width,
                layer.height);
            continue;
        }

        if (!layer.text.texture.valid()) {
            continue;
        }
        renderer.draw(
            layer.text.texture,
            rootX + layer.x,
            rootY + layer.y,
            layer.width,
            layer.height);
    }
}

bool RoleCharacterHud::loadVisualManifest(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    std::ifstream stream(runtimeRoot / L"visual_manifest.tsv", std::ios::binary);
    if (!stream) {
        return false;
    }

    std::string line;
    if (!std::getline(stream, line)) {
        return false;
    }

    std::size_t loadedCount = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto fields = splitTsvLine(line);
        if (fields.size() < 10) {
            continue;
        }

        try {
            const std::string& name = fields[1];
            if (hideWhenNoFreeAttributePoints(name)) {
                continue;
            }

            LayerItem visual;
            visual.kind = LayerKind::Sprite;
            visual.depth = std::stoi(fields[0]);
            visual.x = std::stof(fields[4]);
            visual.y = std::stof(fields[5]);
            visual.width = std::stof(fields[6]);
            visual.height = std::stof(fields[7]);

            if (visual.width <= 0.0F || visual.height <= 0.0F) {
                continue;
            }

            if (!renderer.loadTexture(
                    (runtimeRoot / L"visual" / std::filesystem::path(fields[8])).wstring(),
                    visual.sprite)) {
                continue;
            }

            const int labelDepth = visual.depth + 1;
            const float labelX = visual.x;
            const float labelY = visual.y + 1.0F;
            const float labelWidth = visual.width;
            const float labelHeight = std::min(18.0F, visual.height);
            const wchar_t* label = visualButtonLabel(name);

            layers_.push_back(std::move(visual));
            ++loadedCount;

            if (label != nullptr) {
                addSyntheticText(
                    renderer,
                    labelDepth,
                    label,
                    labelX,
                    labelY,
                    labelWidth,
                    labelHeight,
                    9,
                    TextHorizontalAlign::Center);
            }
        }
        catch (...) {
        }
    }

    return loadedCount > 0;
}

bool RoleCharacterHud::loadTextManifest(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    std::ifstream stream(runtimeRoot / L"text_manifest.tsv", std::ios::binary);
    if (!stream) {
        return false;
    }

    std::string line;
    if (!std::getline(stream, line)) {
        return false;
    }

    std::size_t loadedCount = 0;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto fields = splitTsvLine(line);
        if (fields.size() < 14) {
            continue;
        }

        try {
            const std::wstring_view text = fieldText(fields[1]);
            if (text.empty()) {
                continue;
            }

            LayerItem layer;
            layer.kind = LayerKind::Text;
            layer.depth = std::stoi(fields[0]);
            layer.x = std::stof(fields[3]);
            layer.y = std::stof(fields[4]);
            layer.width = std::stof(fields[5]);
            layer.height = std::stof(fields[6]);

            if (layer.width <= 0.0F || layer.height <= 0.0F) {
                continue;
            }

            TextTextureStyle style;
            style.fontFamily = L"Arial";
            style.fontPixelHeight = std::max<std::uint32_t>(
                1U,
                static_cast<std::uint32_t>(std::lround(std::stof(fields[7]))));
            style.red = static_cast<std::uint8_t>(std::clamp(std::stoi(fields[8]), 0, 255));
            style.green = static_cast<std::uint8_t>(std::clamp(std::stoi(fields[9]), 0, 255));
            style.blue = static_cast<std::uint8_t>(std::clamp(std::stoi(fields[10]), 0, 255));
            style.alpha = static_cast<std::uint8_t>(std::clamp(std::stoi(fields[11]), 0, 255));
            style.align = flashAlign(std::stoi(fields[12]));
            style.wordWrap = fields[13] == "1";

            if (!createTextTexture(
                    renderer,
                    text,
                    std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::ceil(layer.width))),
                    std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::ceil(layer.height))),
                    style,
                    layer.text)) {
                continue;
            }

            layers_.push_back(std::move(layer));
            ++loadedCount;
        }
        catch (...) {
        }
    }

    return loadedCount > 0;
}

void RoleCharacterHud::addSyntheticText(
    SpriteRenderer& renderer,
    const int depth,
    const wchar_t* text,
    const float x,
    const float y,
    const float width,
    const float height,
    const std::uint32_t fontSize,
    const TextHorizontalAlign align,
    const std::uint8_t red,
    const std::uint8_t green,
    const std::uint8_t blue) {
    if (text == nullptr || width <= 0.0F || height <= 0.0F) {
        return;
    }

    LayerItem layer;
    layer.kind = LayerKind::Text;
    layer.depth = depth;
    layer.x = x;
    layer.y = y;
    layer.width = width;
    layer.height = height;

    TextTextureStyle style;
    style.fontFamily = L"Arial";
    style.fontPixelHeight = fontSize;
    style.red = red;
    style.green = green;
    style.blue = blue;
    style.align = align;
    style.wordWrap = false;

    if (createTextTexture(
            renderer,
            text,
            std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::ceil(width))),
            std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::ceil(height))),
            style,
            layer.text)) {
        layers_.push_back(std::move(layer));
    }
}

} // namespace eudoria::game::ui
