#include "beez/core/env_file.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace beez::core
{

namespace
{

struct ParsedEnvLine
{
    std::string key;
    std::string value;
};

[[nodiscard]] std::string trim(std::string_view text)
{
    std::size_t start = 0;
    while (start < text.size() && (text.at(start) == ' ' || text.at(start) == '\t'))
    {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && (text.at(end - 1) == ' ' || text.at(end - 1) == '\t'))
    {
        --end;
    }

    return std::string(text.substr(start, end - start));
}

[[nodiscard]] std::string unquote(std::string_view value)
{
    if (value.size() >= 2)
    {
        const char Quote = value.front();
        if ((Quote == '"' || Quote == '\'') && value.back() == Quote)
        {
            return std::string(value.substr(1, value.size() - 2));
        }
    }
    return std::string(value);
}

[[nodiscard]] std::optional<ParsedEnvLine> parseLine(std::string_view line)
{
    const std::size_t Equals = line.find('=');
    if (Equals == std::string_view::npos)
    {
        return std::nullopt;
    }

    std::string key = trim(line.substr(0, Equals));
    if (key.empty())
    {
        return std::nullopt;
    }

    if (key.starts_with("export "))
    {
        key = trim(key.substr(std::string_view("export ").size()));
    }

    if (key.empty())
    {
        return std::nullopt;
    }

    return ParsedEnvLine {
        .key = std::move(key),
        .value = unquote(trim(line.substr(Equals + 1))),
    };
}

}  // namespace

EnvFile::EnvFile(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<std::string> readProcessEnvironment(const std::string& key)
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c) -- process env lookup for build DSL
    const char* value = std::getenv(key.c_str());
    if (value == nullptr)
    {
        return std::nullopt;
    }
    return std::string(value);
}

void EnvFile::ensureLoaded() const
{
    if (loaded_)
    {
        return;
    }

    loaded_ = true;
    if (!std::filesystem::exists(path_))
    {
        return;
    }

    std::ifstream stream(path_);
    if (!stream.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty() || line.starts_with('#'))
        {
            continue;
        }

        auto parsed = parseLine(line);
        if (!parsed.has_value())
        {
            continue;
        }

        values_.insert_or_assign(std::move(parsed->key), std::move(parsed->value));
    }
}

std::optional<std::string> EnvFile::lookup(const std::string& key) const
{
    ensureLoaded();

    const auto Found = values_.find(key);
    if (Found != values_.end())
    {
        return Found->second;
    }

    return readProcessEnvironment(key);
}

void EnvFile::forEachEntry(
    const std::function<void(const std::string&, const std::string&)>& visitor) const
{
    ensureLoaded();

    for (const auto& [key, value] : values_)
    {
        visitor(key, value);
    }
}

}  // namespace beez::core
