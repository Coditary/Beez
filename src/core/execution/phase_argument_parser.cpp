#include "beez/core/execution/phase_argument_parser.hpp"
#include "beez/core/model/phase_request.hpp"

#include <cctype>
#include <cstddef>
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
    if (position >= input.size())
    {
        return std::nullopt;
    }
    if (input.at(position) != '"')
    {
        return std::nullopt;
    }

    ++position;
    std::string value;
    while (position < input.size())
    {
        const char Character = input.at(position);
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
        while (position < input.size() && (input.at(position) == ' ' || input.at(position) == ','))
        {
            ++position;
        }

        if (position >= input.size())
        {
            break;
        }
        if (input.at(position) == ']')
        {
            break;
        }

        const auto Scope = parseQuotedString(input, position);
        if (!Scope)
        {
            return std::nullopt;
        }

        request.scopes.push_back(*Scope);

        while (position < input.size() && input.at(position) == ' ')
        {
            ++position;
        }

        if (position < input.size() && input.at(position) == ',')
        {
            ++position;
            continue;
        }

        if (position < input.size() && input.at(position) == ']')
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

    const std::string ScopesPart = input.substr(colonPosition + 1);
    if (ScopesPart.empty())
    {
        return std::nullopt;
    }

    std::size_t start = 0;
    while (start < ScopesPart.size())
    {
        const auto CommaPosition = ScopesPart.find(',', start);
        if (CommaPosition != std::string::npos && CommaPosition + 1 >= ScopesPart.size())
        {
            return std::nullopt;
        }

        const auto End = CommaPosition == std::string::npos ? ScopesPart.size() : CommaPosition;
        std::string scope = ScopesPart.substr(start, End - start);

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
