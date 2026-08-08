#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace beez::core
{

class TextTable
{
  public:
    explicit TextTable(std::vector<std::string> headers);

    void addRow(std::vector<std::string> cells);

    void setColumnSpacing(std::size_t spacing);

    [[nodiscard]] std::string format() const;

  private:
    [[nodiscard]] std::vector<std::size_t> columnWidths() const;
    [[nodiscard]] std::string formatSeparator(const std::vector<std::size_t>& widths) const;
    [[nodiscard]] std::string formatRow(const std::vector<std::string>& cells,
                                        const std::vector<std::size_t>& widths) const;

    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    std::size_t columnSpacing_ = 2;
};

}  // namespace beez::core
