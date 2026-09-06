#include "game/data/LegacyTaskCatalog.h"

#include <Windows.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <string_view>

namespace eudoria::game::data {
namespace {

std::vector<std::string_view> splitTsv(const std::string& line) {
    std::vector<std::string_view> columns;
    std::size_t start = 0;
    while (start <= line.size()) {
        const auto end = line.find('\t', start);
        if (end == std::string::npos) {
            columns.emplace_back(line.data() + start, line.size() - start);
            break;
        }
        columns.emplace_back(line.data() + start, end - start);
        start = end + 1;
    }
    return columns;
}

std::wstring fromUtf8(std::string_view text) {
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            required) != required) {
        return {};
    }
    return result;
}

std::int32_t toInt(std::string_view value) noexcept {
    std::int32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} ? result : 0;
}

} // namespace

bool LegacyTaskCatalog::load(const std::filesystem::path& path) {
    tasks_.clear();

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        return false;
    }

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const auto columns = splitTsv(line);
        if (columns.size() < 10) {
            continue;
        }

        LegacyTaskDefinition task;
        task.id = toInt(columns[0]);
        task.kind = toInt(columns[1]);
        task.name = fromUtf8(columns[2]);
        task.brief = fromUtf8(columns[3]);
        task.detail = fromUtf8(columns[4]);
        task.receiveAt = fromUtf8(columns[5]);
        task.finishAt = fromUtf8(columns[6]);
        task.receiveRaw = fromUtf8(columns[7]);
        task.finishRaw = fromUtf8(columns[8]);
        task.starterTyria = columns[9] == "1";

        if (task.id != 0 && !task.name.empty()) {
            tasks_.push_back(std::move(task));
        }
    }

    return !tasks_.empty();
}

const LegacyTaskDefinition* LegacyTaskCatalog::find(const std::int32_t taskId) const noexcept {
    const auto it = std::find_if(tasks_.begin(), tasks_.end(), [taskId](const LegacyTaskDefinition& task) {
        return task.id == taskId;
    });
    return it == tasks_.end() ? nullptr : &*it;
}

std::vector<const LegacyTaskDefinition*> LegacyTaskCatalog::starterUiPreview(const std::size_t maxCount) const {
    std::vector<const LegacyTaskDefinition*> result;
    result.reserve(std::min(maxCount, tasks_.size()));

    for (const auto& task : tasks_) {
        if (task.starterTyria) {
            result.push_back(&task);
        }
    }

    std::sort(result.begin(), result.end(), [](const LegacyTaskDefinition* left, const LegacyTaskDefinition* right) {
        if (left->kind != right->kind) {
            return left->kind < right->kind;
        }
        return left->id < right->id;
    });

    if (result.size() > maxCount) {
        result.resize(maxCount);
    }
    return result;
}

} // namespace eudoria::game::data
