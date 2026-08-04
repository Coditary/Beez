#include "beez/core/phase_argument_parser.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace beez::core
{

namespace
{

[[nodiscard]] std::optional<std::string> parseQuotedString(const std::string& input,
                                                           std::size_t& position)
{
    if (position >= input.size() || input[position] != '"')
    {
        return std::nullopt;
    }

    ++position;
    std::string value;
    while (position < input.size())
    {
        const char Character = input[position];
        if (Character == '"')
        {
            ++position;
            return value;
        }

        value.push_back(Character);
        ++position;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<PhaseRequest> parseBracketPhaseArgument(const std::string& input,
                                                                    std::size_t bracketPosition)
{
    PhaseRequest request;
    request.phase = input.substr(0, bracketPosition);
    if (request.phase.empty())
    {
        return std::nullopt;
    }

    if (input.back() != ']')
    {
        return std::nullopt;
    }

    std::size_t position = bracketPosition + 1;
    while (position < input.size())
    {
        while (position < input.size() && (input[position] == ' ' || input[position] == ','))
        {
            ++position;
        }

        if (position >= input.size() || input[position] == ']')
        {
            break;
        }

        const auto Scope = parseQuotedString(input, position);
        if (!Scope)
        {
            return std::nullopt;
        }

        request.scopes.push_back(*Scope);

        while (position < input.size() && input[position] == ' ')
        {
            ++position;
        }

        if (position < input.size() && input[position] == ',')
        {
            ++position;
            continue;
        }

        if (position < input.size() && input[position] == ']')
        {
            break;
        }

        return std::nullopt;
    }

    return request;
}

[[nodiscard]] std::optional<PhaseRequest> parseColonPhaseArgument(const std::string& input,
                                                                  std::size_t colonPosition)
{
    PhaseRequest request;
    request.phase = input.substr(0, colonPosition);
    if (request.phase.empty())
    {
        return std::nullopt;
    }

    std::string scopesPart = input.substr(colonPosition + 1);
    if (scopesPart.empty())
    {
        return std::nullopt;
    }

    std::size_t start = 0;
    while (start < scopesPart.size())
    {
        const auto CommaPosition = scopesPart.find(',', start);
        if (CommaPosition != std::string::npos && CommaPosition + 1 >= scopesPart.size())
        {
            return std::nullopt;
        }

        const auto End = CommaPosition == std::string::npos ? scopesPart.size() : CommaPosition;
        std::string scope = scopesPart.substr(start, End - start);

        while (!scope.empty() && std::isspace(static_cast<unsigned char>(scope.front())) != 0)
        {
            scope.erase(scope.begin());
        }
        while (!scope.empty() && std::isspace(static_cast<unsigned char>(scope.back())) != 0)
        {
            scope.pop_back();
        }

        if (scope.empty())
        {
            return std::nullopt;
        }

        request.scopes.push_back(scope);

        if (CommaPosition == std::string::npos)
        {
            break;
        }

        start = CommaPosition + 1;
    }

    return request;
}

}  // namespace

std::optional<PhaseRequest> parsePhaseArgument(const std::string& input)
{
    if (input.empty())
    {
        return std::nullopt;
    }

    const auto BracketPosition = input.find('[');
    if (BracketPosition != std::string::npos)
    {
        return parseBracketPhaseArgument(input, BracketPosition);
    }

    const auto ColonPosition = input.find(':');
    if (ColonPosition != std::string::npos)
    {
        return parseColonPhaseArgument(input, ColonPosition);
    }

    return PhaseRequest {.phase = input};
}

}  // namespace beez::core
