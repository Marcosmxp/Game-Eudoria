#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace eudoria::game::data {

struct LegacyTaskDefinition final {
    std::int32_t id = 0;
    std::int32_t kind = 0;
    std::wstring name;
    std::wstring brief;
    std::wstring detail;
    std::wstring receiveAt;
    std::wstring finishAt;
    std::wstring receiveRaw;
    std::wstring finishRaw;
    bool starterTyria = false;
};

class LegacyTaskCatalog final {
public:
    bool load(const std::filesystem::path& path = "legacy_assets/runtime/data/tasks_ui.tsv");

    [[nodiscard]] const std::vector<LegacyTaskDefinition>& tasks() const noexcept { return tasks_; }
    [[nodiscard]] const LegacyTaskDefinition* find(std::int32_t taskId) const noexcept;

    // UI-only bridge used before the real offline player/task state exists.
    // Every returned definition is real txt/itl.json data. The only temporary
    // part is choosing the first Tyria Village main-task definitions for visual
    // validation of the TaskTracer's Available tab.
    [[nodiscard]] std::vector<const LegacyTaskDefinition*> starterUiPreview(std::size_t maxCount = 8) const;

private:
    std::vector<LegacyTaskDefinition> tasks_;
};

} // namespace eudoria::game::data
