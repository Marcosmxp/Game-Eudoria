#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace eudoria::game::ui {

enum class HudWindow : std::size_t {
    Character,
    Pet,
    Mount,
    Wing,
    Inventory,
    Skills,
    Bot,
    Quests,
    Friends,
    Team,
    Guild,
    System,
    Count,
};

[[nodiscard]] constexpr std::string_view hudWindowName(const HudWindow window) noexcept {
    switch (window) {
    case HudWindow::Character: return "character";
    case HudWindow::Pet: return "pet";
    case HudWindow::Mount: return "mount";
    case HudWindow::Wing: return "wing";
    case HudWindow::Inventory: return "inventory";
    case HudWindow::Skills: return "skills";
    case HudWindow::Bot: return "afk";
    case HudWindow::Quests: return "quests";
    case HudWindow::Friends: return "friends";
    case HudWindow::Team: return "team";
    case HudWindow::Guild: return "guild";
    case HudWindow::System: return "system";
    case HudWindow::Count: break;
    }
    return "unknown";
}

class HudWindowManager final {
public:
    void toggle(const HudWindow window) noexcept {
        if (window == HudWindow::Count) {
            return;
        }
        auto& visible = visible_[static_cast<std::size_t>(window)];
        visible = !visible;
    }

    void show(const HudWindow window) noexcept {
        if (window != HudWindow::Count) {
            visible_[static_cast<std::size_t>(window)] = true;
        }
    }

    void hide(const HudWindow window) noexcept {
        if (window != HudWindow::Count) {
            visible_[static_cast<std::size_t>(window)] = false;
        }
    }

    [[nodiscard]] bool visible(const HudWindow window) const noexcept {
        return window != HudWindow::Count && visible_[static_cast<std::size_t>(window)];
    }

private:
    std::array<bool, static_cast<std::size_t>(HudWindow::Count)> visible_{};
};

} // namespace eudoria::game::ui
