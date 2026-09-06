#include "game/ui/RoleCharacterHud.h"

#include <algorithm>
#include <fstream>
#include <sstream>
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

struct MainButtonPlacement final {
    float x;
    float y;
    float scaleX;
    float scaleY;
    const wchar_t* label;
};

constexpr std::array<MainButtonPlacement, 8> kMainButtons{{
    {-207.5F, 189.35F, 0.7500152587890625F, 1.0F, L"Auto"},
    {-128.5F, 189.35F, 0.7499847412109375F, 1.0F, L"OK"},
    {-24.5F, -203.05F, 0.600006103515625F, 1.0F, L"Title"},
    {82.5F, -203.05F, 1.29998779296875F, 1.0F, L"Achievements"},
    {92.3F, -38.0F, 1.2000274658203125F, 0.909088134765625F, L"Nobility"},
    {98.55F, -227.5F, 1.1393585205078125F, 1.0F, L"Arena Rewards"},
    {28.55F, 65.75F, 0.7499847412109375F, 1.0F, L"Synthesize"},
    {-130.65F, -4.65F, 0.800018310546875F, 1.0000152587890625F, L"Gear Evolve"},
}};

struct NamedPayloadChild final {
    std::string name;
    float x = 0.0F;
    float y = 0.0F;
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

std::string lowerAscii(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<NamedPayloadChild> loadNamedPayloadChildren(const std::filesystem::path& runtimeRoot) {
    std::vector<NamedPayloadChild> result;
    std::ifstream stream(runtimeRoot / L"payload.tsv", std::ios::binary);
    if (!stream) {
        return result;
    }

    std::string line;
    std::getline(stream, line); // header
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto fields = splitTsvLine(line);
        if (fields.size() < 6 || fields[1].empty()) {
            continue;
        }

        try {
            NamedPayloadChild child;
            child.name = lowerAscii(fields[1]);
            child.x = std::stof(fields[4]);
            child.y = std::stof(fields[5]);
            result.push_back(std::move(child));
        }
        catch (...) {
        }
    }
    return result;
}

bool findPayloadAnchor(
    const std::vector<NamedPayloadChild>& children,
    const std::initializer_list<const char*> tokens,
    float& x,
    float& y) {
    for (const auto& child : children) {
        for (const char* token : tokens) {
            if (child.name.find(token) != std::string::npos) {
                x = child.x;
                y = child.y;
                return true;
            }
        }
    }
    return false;
}

} // namespace

bool RoleCharacterHud::initialize(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    bool loaded = true;
    loaded = renderer.loadTexture((runtimeRoot / L"equipment_panel.png").wstring(), equipmentPanel_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"equipment_slot.png").wstring(), equipmentSlot_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"panel.png").wstring(), panel_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"value_back.png").wstring(), valueBack_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"progress" / L"100.png").wstring(), progress100_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"attr_add" / L"up.png").wstring(), attrAdd_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"attr_remove" / L"up.png").wstring(), attrRemove_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"attr_add_all" / L"up.png").wstring(), attrAddAll_) && loaded;
    loaded = renderer.loadTexture((runtimeRoot / L"main_button.png").wstring(), mainButton_) && loaded;

    // Optional exact static extras recovered from symbol1998. The stable core
    // above remains authoritative, so a missing manifest never blocks startup.
    loadAutoVisuals(renderer, runtimeRoot);

    texts_.clear();

    // Equipment slot captions. Exact strings come from txt/idc.json -> RoleUI;
    // positions are DefineEditText placements recovered from symbol1998.
    addText(renderer, L"Sperion", -24.0F, -138.65F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Neck", -24.0F, -24.65F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Ring", -24.0F, 13.35F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Wings", -24.0F, -62.65F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Insig", -24.0F, -100.65F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Mount", -24.0F, 51.35F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Helm", -205.45F, -138.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Weap", -205.45F, 52.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Offhand", -205.45F, 14.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Armor", -205.45F, -100.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Gloves", -205.45F, -62.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Boots", -205.45F, -24.0F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Hat", -130.5F, 45.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Coat", -88.5F, 45.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Banner", -66.5F, -138.35F, 45.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Cape", -154.5F, -138.0F, 37.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Talisman", 44.5F, 29.35F, 40.0F, 8, TextHorizontalAlign::Center);
    addText(renderer, L"Artefact", -154.0F, -100.35F, 39.0F, 8, TextHorizontalAlign::Center);

    // Identity/progression panel. The source screenshot exposed clipping on the
    // far-left Title/EXP labels; keep them inside the symbol1998 content area.
    // Values remain neutral until PlayerState is implemented.
    addText(renderer, L"Level:", 42.0F, -176.05F, 86.0F);
    addText(renderer, L"1", 129.0F, -177.05F, 80.0F);
    addText(renderer, L"EXP:", -202.0F, -176.0F, 48.0F);
    addText(renderer, L"0 / 0", -152.5F, -176.0F, 146.0F);
    addText(renderer, L"Title:", -202.0F, -198.0F, 48.0F);
    addText(renderer, L"-", -153.0F, -198.0F, 124.0F);
    addText(renderer, L"Class:", 42.0F, -146.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -146.0F, 79.0F);
    addText(renderer, L"Plane:", 42.0F, -116.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -116.0F, 79.0F);
    addText(renderer, L"Rank:", 42.0F, -85.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -85.0F, 79.0F);
    addText(renderer, L"Honor:", 42.0F, -55.0F, 86.0F);
    addText(renderer, L"0", 134.85F, -55.0F, 75.5F);
    addText(renderer, L"Guild:", 42.0F, -1.0F, 86.0F);
    addText(renderer, L"-", 129.0F, -1.0F, 79.0F);
    addText(renderer, L"PK Lvl:", 42.0F, 34.0F, 86.0F);
    addText(renderer, L"0", 129.0F, 34.0F, 41.0F);
    addText(renderer, L"Arena Score:", -208.45F, -220.0F, 80.0F, 9);
    addText(renderer, L"0", -128.4F, -220.0F, 77.0F, 9);
    addText(renderer, L"Current Tier:", -50.45F, -220.0F, 80.0F, 9);
    addText(renderer, L"-", 27.6F, -220.0F, 76.0F, 9);

    // The reference client exposes these feature captions around dynamically
    // populated icons. We first try to anchor each caption to the exact named
    // display-list child emitted by payload.tsv. If a feature is created later
    // by ActionScript and therefore has no first-frame child, the fallback only
    // reserves its visual label; no fake icon or gameplay behavior is created.
    const auto namedChildren = loadNamedPayloadChildren(runtimeRoot);
    struct FeatureLabelSpec final {
        const wchar_t* label;
        std::initializer_list<const char*> tokens;
        float fallbackX;
        float fallbackY;
        float width;
    };
    const FeatureLabelSpec featureLabels[] = {
        {L"Wings", {"wing"}, 35.0F, -132.0F, 55.0F},
        {L"Etherealization", {"ether", "ethereal"}, 68.0F, -132.0F, 72.0F},
        {L"Cape", {"cape", "cloak"}, 42.0F, -88.0F, 56.0F},
        {L"Legend Gem", {"legendgem", "legend_gem", "gem"}, 42.0F, -45.0F, 72.0F},
        {L"Wunderkind", {"wunder", "wonder"}, 42.0F, -4.0F, 72.0F},
        {L"Soul", {"soul"}, 111.0F, 66.0F, 55.0F},
    };
    for (const auto& spec : featureLabels) {
        float anchorX = spec.fallbackX;
        float anchorY = spec.fallbackY;
        if (findPayloadAnchor(namedChildren, spec.tokens, anchorX, anchorY)) {
            addText(renderer, spec.label, anchorX - 24.0F, anchorY + 18.0F, spec.width, 8, TextHorizontalAlign::Center);
        }
        else {
            addText(renderer, spec.label, spec.fallbackX, spec.fallbackY, spec.width, 8, TextHorizontalAlign::Center);
        }
    }

    // Visible in the live Character panel directly below Gear Evolve. Costume
    // switching itself is gameplay/player-appearance state and remains deferred.
    addText(renderer, L"Show Costume", -157.0F, 18.0F, 96.0F, 8, TextHorizontalAlign::Center);

    // Primary attributes.
    addText(renderer, L"Strength:", -215.5F, 95.0F, 71.0F);
    addText(renderer, L"Agility:", -215.5F, 114.0F, 71.0F);
    addText(renderer, L"Intellect:", -216.0F, 133.0F, 71.0F);
    addText(renderer, L"Endurance:", -216.0F, 152.0F, 71.0F);
    addText(renderer, L"0", -144.5F, 95.0F, 43.0F);
    addText(renderer, L"0", -144.5F, 114.0F, 42.0F);
    addText(renderer, L"0", -144.5F, 133.0F, 44.0F);
    addText(renderer, L"0", -144.5F, 152.0F, 44.0F);
    addText(renderer, L"Points:", -214.5F, 170.95F, 70.0F);
    addText(renderer, L"0", -145.0F, 170.95F, 45.0F);

    // Core combat/resources panel.
    addText(renderer, L"HP:", -40.0F, 98.0F, 56.0F);
    addText(renderer, L"0 / 0", 15.5F, 98.0F, 146.0F);
    addText(renderer, L"MP:", -40.0F, 114.0F, 56.0F);
    addText(renderer, L"0 / 0", 15.5F, 114.0F, 65.0F);
    addText(renderer, L"Crystal Essence:", -40.0F, 130.0F, 56.0F, 8);
    addText(renderer, L"0", 15.5F, 130.0F, 67.0F);
    addText(renderer, L"PATK:", -39.7F, 146.0F, 56.0F);
    addText(renderer, L"0", 15.0F, 146.0F, 66.0F);
    addText(renderer, L"MATK:", -39.7F, 162.0F, 56.0F);
    addText(renderer, L"0", 15.0F, 162.0F, 67.0F);
    addText(renderer, L"Hit:", -39.7F, 178.0F, 56.0F);
    addText(renderer, L"0", 15.0F, 178.0F, 67.0F);
    addText(renderer, L"Heal:", -39.7F, 194.0F, 56.0F);
    addText(renderer, L"0", 15.0F, 194.0F, 67.0F);
    addText(renderer, L"PDEF:", 81.0F, 146.0F, 56.0F);
    addText(renderer, L"0", 137.0F, 146.0F, 74.0F);
    addText(renderer, L"MDEF:", 81.0F, 162.0F, 56.0F);
    addText(renderer, L"0", 137.0F, 162.0F, 74.0F);
    addText(renderer, L"Dodge:", 81.0F, 178.0F, 56.0F);
    addText(renderer, L"0", 137.0F, 178.0F, 74.0F);
    addText(renderer, L"Crit:", 81.0F, 194.0F, 56.0F);
    addText(renderer, L"0", 137.0F, 194.0F, 74.0F);
    addText(renderer, L"Charm:", 72.0F, 114.0F, 65.0F, 9);
    addText(renderer, L"0", 137.0F, 114.55F, 65.0F, 9);
    addText(renderer, L"Prestige:", 82.0F, 130.7F, 55.0F, 9);
    addText(renderer, L"0", 137.0F, 130.7F, 74.0F, 9);

    // Bonus stats panel.
    const struct BonusSpec { const wchar_t* name; float lx; float y; float vx; } bonus[] = {
        {L"Atk Spd:",-205.0F,224.0F,-124.0F}, {L"Cast Spd:",-205.0F,240.0F,-124.0F},
        {L"Move Spd:",-204.5F,256.0F,-123.5F}, {L"HP Regen:",-63.5F,224.55F,18.5F},
        {L"MP Regen:",-63.5F,240.55F,18.5F}, {L"Luck:",-63.5F,256.55F,18.5F},
        {L"Luck Def:",-63.5F,272.55F,18.5F}, {L"Crit Dmg:",71.5F,224.55F,153.5F},
        {L"Crit Def:",71.5F,240.55F,153.5F}, {L"AP:",71.5F,256.55F,153.5F},
        {L"AP Def:",71.5F,272.55F,153.5F},
    };
    for (const auto& spec : bonus) {
        addText(renderer, spec.name, spec.lx, spec.y, 82.0F, 9);
        addText(renderer, L"0", spec.vx, spec.y, 52.0F, 9);
    }

    addText(renderer, L"LOT:", -179.2F, 302.15F, 43.0F, 9);
    addText(renderer, L"0", -136.65F, 302.15F, 82.0F, 9);
    addText(renderer, L"LOA:", -55.7F, 302.15F, 43.0F, 9);
    addText(renderer, L"0", -13.7F, 302.15F, 82.0F, 9);
    addText(renderer, L"LOH:", 67.3F, 302.15F, 43.0F, 9);
    addText(renderer, L"0", 109.3F, 302.15F, 82.0F, 9);

    for (const auto& button : kMainButtons) {
        addText(
            renderer,
            button.label,
            button.x,
            button.y + 2.0F,
            100.0F * button.scaleX,
            9,
            TextHorizontalAlign::Center);
    }

    return loaded;
}

void RoleCharacterHud::render(
    SpriteRenderer& renderer,
    const float rootX,
    const float rootY) const {
    // depth 1: shape1884 at (-25,11)
    if (equipmentPanel_.valid()) {
        renderer.draw(
            equipmentPanel_,
            rootX - 25.0F - 186.75F,
            rootY + 11.0F - 164.75F,
            237.5F,
            239.5F);
    }

    // Equipment IconBarMC instances use symbol276 bounds -3..35.
    for (const auto& slot : kEquipmentSlots) {
        drawSpriteAtBounds(
            renderer,
            equipmentSlot_,
            rootX,
            rootY,
            slot,
            -3.0F,
            -3.0F,
            38.0F,
            38.0F);
    }

    // Generic panel character304 bounds -24..24.
    for (const auto& panel : kPanels) {
        drawSpriteAtBounds(
            renderer,
            panel_,
            rootX,
            rootY,
            panel,
            -24.0F,
            -24.0F,
            48.0F,
            48.0F);
    }

    // Generic value backing character263 bounds -21..21.
    for (const auto& back : kValueBacks) {
        drawSpriteAtBounds(
            renderer,
            valueBack_,
            rootX,
            rootY,
            back,
            -21.0F,
            -21.0F,
            42.0F,
            42.0F);
    }

    // expBar and the secondary 100-frame meter are initialized to frame 100
    // by the original PlayerFullInfo controller. symbol361 bounds are exact.
    constexpr float progressLeft = -250.49958038330078F;
    constexpr float progressTop = -12.49652099609375F;
    constexpr float progressWidth = 327.0F;
    constexpr float progressHeight = 25.001155853271484F;
    if (progress100_.valid()) {
        drawSpriteAtBounds(
            renderer,
            progress100_,
            rootX,
            rootY,
            {-79.5F,-168.05F,1.0F,1.0F},
            progressLeft,
            progressTop,
            progressWidth,
            progressHeight);
        drawSpriteAtBounds(
            renderer,
            progress100_,
            rootX,
            rootY,
            {170.5F,-47.0F,0.5320892333984375F,1.0F},
            progressLeft,
            progressTop,
            progressWidth,
            progressHeight);
    }

    // Static extras discovered from the actual symbol1998 display list. Their
    // geometry comes from assets.swf and the extracted PNG is the corresponding
    // FFDec character asset. This intentionally does not fabricate the central
    // player model or equipped item contents: those are dynamic runtime objects.
    for (const auto& visual : autoVisuals_) {
        if (!visual.texture.valid()) {
            continue;
        }
        renderer.draw(
            visual.texture,
            rootX + visual.x,
            rootY + visual.y,
            visual.width,
            visual.height);
    }

    // Attribute allocation controls. Gameplay point mutation is deliberately
    // deferred; these are the real payload visuals in their original positions.
    for (const auto& placement : kAttrAddAll) {
        drawSpriteAtBounds(renderer, attrAddAll_, rootX, rootY, placement, -6.0F, -6.0F, 12.0F, 12.0F);
    }
    for (const auto& placement : kAttrAdd) {
        drawSpriteAtBounds(renderer, attrAdd_, rootX, rootY, placement, -6.0F, -6.0F, 12.0F, 12.0F);
    }
    for (const auto& placement : kAttrRemove) {
        drawSpriteAtBounds(renderer, attrRemove_, rootX, rootY, placement, -6.0F, -6.0F, 12.0F, 12.0F);
    }

    // MainButton character89 has local bounds 0..100 x 0..22.
    if (mainButton_.valid()) {
        for (const auto& button : kMainButtons) {
            renderer.draw(
                mainButton_,
                rootX + button.x,
                rootY + button.y,
                100.0F * button.scaleX,
                22.0F * button.scaleY);
        }
    }

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

void RoleCharacterHud::loadAutoVisuals(
    SpriteRenderer& renderer,
    const std::filesystem::path& runtimeRoot) {
    autoVisuals_.clear();

    const auto manifestPath = runtimeRoot / L"auto_manifest.tsv";
    std::ifstream stream(manifestPath, std::ios::binary);
    if (!stream) {
        return;
    }

    std::string line;
    if (!std::getline(stream, line)) {
        return;
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto fields = splitTsvLine(line);
        if (fields.size() < 10 || fields[8].empty()) {
            continue;
        }

        try {
            AutoVisual visual;
            visual.depth = std::stoi(fields[0]);
            visual.x = std::stof(fields[4]);
            visual.y = std::stof(fields[5]);
            visual.width = std::stof(fields[6]);
            visual.height = std::stof(fields[7]);

            if (visual.width == 0.0F || visual.height == 0.0F) {
                continue;
            }

            if (!renderer.loadTexture(
                    (runtimeRoot / L"auto" / std::filesystem::path(fields[8])).wstring(),
                    visual.texture)) {
                continue;
            }

            autoVisuals_.push_back(std::move(visual));
        }
        catch (...) {
            // Malformed optional auto entry must never prevent the stable Role
            // window from loading.
        }
    }

    std::sort(
        autoVisuals_.begin(),
        autoVisuals_.end(),
        [](const AutoVisual& left, const AutoVisual& right) {
            return left.depth < right.depth;
        });
}

void RoleCharacterHud::drawSpriteAtBounds(
    SpriteRenderer& renderer,
    const SpriteTexture& texture,
    const float rootX,
    const float rootY,
    const TexturePlacement& placement,
    const float boundsLeft,
    const float boundsTop,
    const float boundsWidth,
    const float boundsHeight) const {
    if (!texture.valid()) {
        return;
    }

    renderer.draw(
        texture,
        rootX + placement.x + (boundsLeft * placement.scaleX),
        rootY + placement.y + (boundsTop * placement.scaleY),
        boundsWidth * placement.scaleX,
        boundsHeight * placement.scaleY);
}

} // namespace eudoria::game::ui
