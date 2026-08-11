#include "beez/plugin/lua/api/data/detail/csv_convert.hpp"

#include "beez/plugin/lua/api/data/detail/lua_convert.hpp"

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fast-cpp-csv-parser/csv.h>

// NOLINTBEGIN(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-container-size-empty)
#include <sol/sol.hpp>

namespace beez::plugin::lua::data_detail
{

namespace
{

[[nodiscard]] std::vector<std::string> parseCsvLineFields(char* line)
{
    using QuotePolicy = io::no_quote_escape<','>;
    using TrimPolicy = io::trim_chars<' ', '\t'>;

    std::vector<std::string> fields;
    while (line != nullptr)
    {
        char* columnBegin = nullptr;
        char* columnEnd = nullptr;
        io::detail::chop_next_column<QuotePolicy>(line, columnBegin, columnEnd);
        TrimPolicy::trim(columnBegin, columnEnd);
        QuotePolicy::unescape(columnBegin, columnEnd);
        fields.emplace_back(columnBegin);
    }

    return fields;
}

[[nodiscard]] std::string escapeCsvField(const std::string& field)
{
    const bool NeedsQuotes =
        field.find(',') != std::string::npos || field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos || field.find('\r') != std::string::npos;
    if (!NeedsQuotes)
    {
        return field;
    }

    std::string escaped;
    escaped.reserve(field.size() + 2);
    escaped.push_back('"');
    for (const char Character : field)
    {
        if (Character == '"')
        {
            escaped.append("\"\"");
            continue;
        }

        escaped.push_back(Character);
    }
    escaped.push_back('"');
    return escaped;
}

[[nodiscard]] bool isRowTable(const sol::table& table)
{
    if (table.empty())
    {
        return false;
    }

    for (std::size_t index = 1; index <= table.size(); ++index)
    {
        const sol::object Value = table[index];
        if (!Value.valid())
        {
            return false;
        }
    }

    return true;
}

}  // namespace

sol::table csvStringToLua(sol::state_view luaState, const std::string& content)
{
    std::istringstream stream(content);
    io::LineReader reader("csv", stream);

    sol::table rows = luaState.create_table();
    std::size_t rowIndex = 1;

    while (char* line = reader.next_line())
    {
        const std::string_view LineView(line);
        if (LineView.empty() || LineView.front() == '#')
        {
            continue;
        }

        const std::vector<std::string> Fields = parseCsvLineFields(line);
        sol::table row = luaState.create_table();
        for (std::size_t index = 0; index < Fields.size(); ++index)
        {
            row[index + 1] = Fields.at(index);
        }
        rows[rowIndex] = row;
        ++rowIndex;
    }

    return rows;
}

std::string luaTableToCsvString(const sol::table& table)
{
    if (!isLuaArray(table))
    {
        throw std::runtime_error("beez.data: CSV serialization expects an array of row tables");
    }

    std::ostringstream output;
    for (std::size_t rowIndex = 1; rowIndex <= table.size(); ++rowIndex)
    {
        const sol::object RowValue = table[rowIndex];
        if (!RowValue.valid() || !RowValue.is<sol::table>())
        {
            throw std::runtime_error("beez.data: CSV rows must be tables");
        }

        const sol::table Row = RowValue.as<sol::table>();
        if (!isRowTable(Row))
        {
            throw std::runtime_error("beez.data: CSV rows must be sequential 1-based arrays");
        }

        for (std::size_t columnIndex = 1; columnIndex <= Row.size(); ++columnIndex)
        {
            if (columnIndex > 1)
            {
                output << ',';
            }

            const sol::object Cell = Row[columnIndex];
            if (!Cell.valid() || !Cell.is<std::string>())
            {
                throw std::runtime_error("beez.data: CSV cells must be strings");
            }

            output << escapeCsvField(Cell.as<std::string>());
        }

        output << '\n';
    }

    return output.str();
}

}  // namespace beez::plugin::lua::data_detail
// NOLINTEND(misc-include-cleaner,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-container-size-empty)
