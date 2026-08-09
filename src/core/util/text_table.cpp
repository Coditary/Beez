#include "beez/core/util/text_table.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace beez::core
{

TextTable::TextTable(std::vector<std::string> headers) : headers_(std::move(headers)) {}

void TextTable::addRow(std::vector<std::string> cells)
{
    if (cells.size() < headers_.size())
    {
        cells.resize(headers_.size());
    }

    rows_.push_back(std::move(cells));
}

void TextTable::setColumnSpacing(const std::size_t Spacing)
{
    columnSpacing_ = Spacing;
}

std::vector<std::size_t> TextTable::columnWidths() const
{
    std::vector<std::size_t> widths;
    widths.reserve(headers_.size());

    for (std::size_t column = 0; column < headers_.size(); ++column)
    {
        std::size_t width = headers_.at(column).size();
        for (const auto& row : rows_)
        {
            if (column < row.size())
            {
                width = std::max(width, row.at(column).size());
            }
        }

        widths.push_back(width);
    }

    return widths;
}

std::string TextTable::formatSeparator(const std::vector<std::size_t>& widths) const
{
    std::ostringstream stream;
    for (std::size_t column = 0; column < widths.size(); ++column)
    {
        if (column > 0)
        {
            stream << std::string(columnSpacing_, ' ');
        }

        stream << std::string(widths.at(column), '-');
    }

    return stream.str();
}

std::string TextTable::formatRow(const std::vector<std::string>& cells,
                                 const std::vector<std::size_t>& widths) const
{
    std::ostringstream stream;
    for (std::size_t column = 0; column < widths.size(); ++column)
    {
        if (column > 0)
        {
            stream << std::string(columnSpacing_, ' ');
        }

        const std::string& cell = column < cells.size() ? cells.at(column) : std::string {};
        stream << std::left << std::setw(static_cast<int>(widths.at(column))) << cell;
    }

    return stream.str();
}

std::string TextTable::format() const
{
    if (headers_.empty())
    {
        return {};
    }

    const auto Widths = columnWidths();
    std::ostringstream stream;
    stream << formatRow(headers_, Widths) << '\n';
    stream << formatSeparator(Widths) << '\n';

    for (const auto& row : rows_)
    {
        stream << formatRow(row, Widths) << '\n';
    }

    std::string output = stream.str();
    if (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}

}  // namespace beez::core
