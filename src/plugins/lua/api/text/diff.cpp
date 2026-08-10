#include "beez/plugin/lua/api/text/diff.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#include <sol/sol.hpp>
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace beez::plugin::lua
{

namespace
{

struct TextDiffChunk
{
    std::string op;
    std::string text;
};

[[nodiscard]] std::vector<std::string> splitLines(std::string_view text)
{
    std::vector<std::string> lines;
    if (text.empty())
    {
        lines.emplace_back();
        return lines;
    }

    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t newline = text.find('\n', start);
        if (newline == std::string_view::npos)
        {
            lines.emplace_back(text.substr(start));
            break;
        }

        lines.emplace_back(text.substr(start, newline - start));
        start = newline + 1;
        if (start == text.size())
        {
            lines.emplace_back();
            break;
        }
    }

    return lines;
}

[[nodiscard]] std::vector<std::size_t> longestCommonSubsequenceTable(const std::vector<std::string>& left,
                                                                     const std::vector<std::string>& right)
{
    const std::size_t leftSize = left.size();
    const std::size_t rightSize = right.size();
    std::vector<std::size_t> table((leftSize + 1U) * (rightSize + 1U), 0U);

    for (std::size_t i = 1; i <= leftSize; ++i)
    {
        for (std::size_t j = 1; j <= rightSize; ++j)
        {
            const std::size_t index = (i * (rightSize + 1U)) + j;
            if (left[i - 1U] == right[j - 1U])
            {
                table[index] = table[((i - 1U) * (rightSize + 1U)) + (j - 1U)] + 1U;
            }
            else
            {
                table[index] = std::max(table[((i - 1U) * (rightSize + 1U)) + j],
                                      table[(i * (rightSize + 1U)) + (j - 1U)]);
            }
        }
    }

    return table;
}

[[nodiscard]] std::vector<TextDiffChunk> diffText(std::string_view oldText, std::string_view newText)
{
    const std::vector<std::string> oldLines = splitLines(oldText);
    const std::vector<std::string> newLines = splitLines(newText);
    const std::vector<std::size_t> table = longestCommonSubsequenceTable(oldLines, newLines);

    std::vector<TextDiffChunk> chunks;
    std::size_t leftIndex = oldLines.size();
    std::size_t rightIndex = newLines.size();

    while (leftIndex > 0 || rightIndex > 0)
    {
        if (leftIndex > 0 && rightIndex > 0 && oldLines[leftIndex - 1U] == newLines[rightIndex - 1U])
        {
            chunks.push_back(TextDiffChunk{.op = "equal", .text = oldLines[leftIndex - 1U]});
            --leftIndex;
            --rightIndex;
            continue;
        }

        const std::size_t rightSize = newLines.size();
        const std::size_t up = (leftIndex > 0)
                                   ? table[((leftIndex - 1U) * (rightSize + 1U)) + rightIndex]
                                   : 0U;
        const std::size_t left = (rightIndex > 0) ? table[(leftIndex * (rightSize + 1U)) + (rightIndex - 1U)] : 0U;

        if (leftIndex > 0 && (rightIndex == 0 || up >= left))
        {
            chunks.push_back(TextDiffChunk{.op = "delete", .text = oldLines[leftIndex - 1U]});
            --leftIndex;
        }
        else
        {
            chunks.push_back(TextDiffChunk{.op = "insert", .text = newLines[rightIndex - 1U]});
            --rightIndex;
        }
    }

    std::ranges::reverse(chunks);
    return chunks;
}

}  // namespace

void bindDiff(sol::table& textTable, const std::shared_ptr<sol::state>& luaState)
{
    textTable["diff"] = [luaState](const std::string& oldText, const std::string& newText) -> sol::table
    {
        const std::vector<TextDiffChunk> chunks = diffText(oldText, newText);
        sol::table result = luaState->create_table(static_cast<int>(chunks.size()), 0);
        for (std::size_t index = 0; index < chunks.size(); ++index)
        {
            sol::table entry = luaState->create_table();
            entry["op"] = chunks[index].op;
            entry["text"] = chunks[index].text;
            result[index + 1U] = entry;
        }

        return result;
    };
}

}  // namespace beez::plugin::lua
