// NOLINTBEGIN(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
#include "beez/core/config/ui/theme.hpp"

#include <array>
#include <cctype>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace beez::core
{

namespace
{

[[nodiscard]] bool isHexDigit(const char character)
{
    return std::isxdigit(static_cast<unsigned char>(character)) != 0;
}

[[nodiscard]] std::optional<std::uint8_t> parseHexByte(std::string_view text)
{
    if (text.size() != 2 || !isHexDigit(text[0]) || !isHexDigit(text[1]))
    {
        return std::nullopt;
    }

    const auto parseNibble = [](const char character) -> std::uint8_t
    {
        if (character >= '0' && character <= '9')
        {
            return static_cast<std::uint8_t>(character - '0');
        }

        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        return static_cast<std::uint8_t>(lower - 'a' + 10);
    };

    return static_cast<std::uint8_t>((parseNibble(text[0]) << 4U) | parseNibble(text[1]));
}

[[nodiscard]] std::optional<std::array<std::uint8_t, 3>> parseHexColor(std::string_view value)
{
    if (value.empty())
    {
        return std::nullopt;
    }

    if (value.front() == '#')
    {
        value.remove_prefix(1);
    }

    if (value.size() != 6)
    {
        return std::nullopt;
    }

    const auto red = parseHexByte(value.substr(0, 2));
    const auto green = parseHexByte(value.substr(2, 2));
    const auto blue = parseHexByte(value.substr(4, 2));
    if (!red.has_value() || !green.has_value() || !blue.has_value())
    {
        return std::nullopt;
    }

    return std::array<std::uint8_t, 3> {*red, *green, *blue};
}

[[nodiscard]] std::string trueColorSequence(const std::array<std::uint8_t, 3>& rgb)
{
    return "\033[38;2;" + std::to_string(rgb[0]) + ';' + std::to_string(rgb[1]) + ';' +
           std::to_string(rgb[2]) + 'm';
}

[[nodiscard]] std::string basicColorSequence(const std::array<std::uint8_t, 3>& rgb)
{
    const int luminance = (299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]) / 1000;
    if (luminance >= 180)
    {
        return "\033[97m";
    }
    if (luminance >= 120)
    {
        return "\033[37m";
    }
    if (luminance >= 80)
    {
        return "\033[90m";
    }

    if (rgb[0] > rgb[1] && rgb[0] > rgb[2])
    {
        return "\033[31m";
    }
    if (rgb[1] > rgb[0] && rgb[1] > rgb[2])
    {
        return "\033[32m";
    }
    if (rgb[2] > rgb[0] && rgb[2] > rgb[1])
    {
        return "\033[34m";
    }

    return "\033[36m";
}

[[nodiscard]] std::string ansiForeground(const UiSettings& settings, const std::string& hexColor)
{
    const auto rgb = parseHexColor(hexColor);
    if (!rgb.has_value())
    {
        return {};
    }

    if (settings.truecolor)
    {
        return trueColorSequence(*rgb);
    }

    return basicColorSequence(*rgb);
}

}  // namespace

UiColorPalette defaultColorPalette()
{
    return UiColorPalette {
        .text = "#d4d4d4",
        .muted = "#808080",
        .success = "#33cc33",
        .warning = "#e6c200",
        .error = "#e05252",
        .info = "#4da3ff",
        .accent = "#2ec4c4",
        .progressFill = "#33cc33",
        .progressEmpty = "#555555",
        .cacheHit = "#6fbf6f",
        .workerPrefix = "#5aa9ff",
    };
}

UiColorPalette resolveThemePalette(const std::optional<std::string>& themeName,
                                   const std::map<std::string, UiColorPalette>& themes)
{
    if (!themeName.has_value())
    {
        return {};
    }

    const auto found = themes.find(*themeName);
    if (found == themes.end())
    {
        throw std::runtime_error("unknown ui theme: " + *themeName);
    }

    return found->second;
}

std::string
colorizeText(const UiSettings& settings, const std::string& hexColor, const std::string_view text)
{
    if (!settings.colors || text.empty())
    {
        return std::string(text);
    }

    const std::string sequence = ansiForeground(settings, hexColor);
    if (sequence.empty())
    {
        return std::string(text);
    }

    return sequence + std::string(text) + "\033[0m";
}

}  // namespace beez::core
// NOLINTEND(misc-include-cleaner,readability-identifier-naming,readability-math-missing-parentheses,performance-no-automatic-move,modernize-return-braced-init-list,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-avoid-magic-numbers,bugprone-easily-swappable-parameters,readability-avoid-nested-conditional-operator)
