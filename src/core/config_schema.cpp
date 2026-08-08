#include "beez/core/config_schema.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/text_table.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beez::core
{

namespace
{

constexpr std::int64_t MaxConfiguredWorkerThreads = 1024;

enum class ConfigSchemaKind : std::uint8_t
{
    Object,
    Enum,
    Boolean,
    Number,
    String,
    Path,
    StringMap,
};

struct NumberConstraints
{
    std::optional<std::int64_t> min;
    std::optional<std::int64_t> max;
    std::string storage = "integer";
    std::optional<std::string> defaultValue;
};

struct ConfigSchemaNode
{
    ConfigSchemaKind kind = ConfigSchemaKind::Object;
    std::vector<std::string> enumValues;
    std::map<std::string, ConfigSchemaNode> children;
    std::optional<NumberConstraints> number;
};

[[nodiscard]] std::vector<std::string> toStringVector(const std::vector<const char*>& values)
{
    std::vector<std::string> result;
    result.reserve(values.size());
    std::ranges::transform(
        values, std::back_inserter(result), [](const char* value) { return std::string(value); });
    return result;
}

[[nodiscard]] ConfigSchemaNode makeEnumNode(const std::vector<const char*>& values)
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::Enum;
    node.enumValues = toStringVector(values);
    return node;
}

[[nodiscard]] ConfigSchemaNode makeBooleanNode()
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::Boolean;
    return node;
}

[[nodiscard]] ConfigSchemaNode makeStringNode()
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::String;
    return node;
}

[[nodiscard]] ConfigSchemaNode makePathNode()
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::Path;
    return node;
}

[[nodiscard]] ConfigSchemaNode makeNumberNode(NumberConstraints constraints)
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::Number;
    node.number = std::move(constraints);
    return node;
}

[[nodiscard]] ConfigSchemaNode makeStringMapNode()
{
    ConfigSchemaNode node;
    node.kind = ConfigSchemaKind::StringMap;
    return node;
}

[[nodiscard]] const ConfigSchemaNode& rootConfigSchema()
{
    static const ConfigSchemaNode Root = []()
    {
        ConfigSchemaNode root;
        root.kind = ConfigSchemaKind::Object;

        root.children.emplace(
            "performance",
            []()
            {
                ConfigSchemaNode performance;
                performance.kind = ConfigSchemaKind::Object;
                performance.children.emplace("max_threads",
                                             makeNumberNode(NumberConstraints {
                                                 .min = 1,
                                                 .max = MaxConfiguredWorkerThreads,
                                                 .storage = "integer",
                                                 .defaultValue = "CPU core count",
                                             }));
                performance.children.emplace("cache_write_strategy",
                                             makeEnumNode(cacheWriteStrategyNames()));
                performance.children.emplace("cache_fs_metadata", makeBooleanNode());
                performance.children.emplace("use_mmap_for_hashing", makeBooleanNode());
                performance.children.emplace("mmap_hashing_min_bytes",
                                             makeNumberNode(NumberConstraints {
                                                 .min = 1,
                                                 .storage = "integer",
                                                 .defaultValue = "65536",
                                             }));
                performance.children.emplace("optimize_gc_for_throughput", makeBooleanNode());
                performance.children.emplace("pin_threads_to_cores", makeBooleanNode());
                return performance;
            }());

        root.children.emplace(
            "cache",
            []()
            {
                ConfigSchemaNode cache;
                cache.kind = ConfigSchemaKind::Object;
                cache.children.emplace("path", makePathNode());
                cache.children.emplace("enabled", makeBooleanNode());
                cache.children.emplace("protect", makeBooleanNode());

                ConfigSchemaNode hash;
                hash.kind = ConfigSchemaKind::Object;
                hash.children.emplace("algorithm", makeEnumNode(contentHashAlgorithmNames()));
                hash.children.emplace("seed",
                                      makeNumberNode(NumberConstraints {
                                          .min = 0,
                                          .storage = "uint32",
                                          .defaultValue = "0",
                                      }));
                cache.children.emplace("hash", std::move(hash));

                ConfigSchemaNode compress;
                compress.kind = ConfigSchemaKind::Object;
                compress.children.emplace("algorithm",
                                          makeEnumNode(cacheCompressionAlgorithmNames()));
                compress.children.emplace(
                    "level",
                    makeNumberNode(NumberConstraints {
                        .min = 0,
                        .max = MaxCacheCompressionLevel,
                        .storage = "integer",
                        .defaultValue = std::to_string(DefaultCacheCompressionLevel),
                    }));
                compress.children.emplace("mode", makeEnumNode(cacheCompressionModeNames()));
                cache.children.emplace("compress", std::move(compress));

                return cache;
            }());

        root.children.emplace("ui",
                              []()
                              {
                                  ConfigSchemaNode uiSettings;
                                  uiSettings.kind = ConfigSchemaKind::Object;
                                  uiSettings.children.emplace("output_mode",
                                                              makeEnumNode({"clean", "verbose"}));
                                  return uiSettings;
                              }());

        root.children.emplace("paths",
                              []()
                              {
                                  ConfigSchemaNode paths;
                                  paths.kind = ConfigSchemaKind::Object;
                                  paths.children.emplace("env_file", makePathNode());
                                  paths.children.emplace("build_script", makeStringNode());
                                  return paths;
                              }());

        root.children.emplace("engine",
                              []()
                              {
                                  ConfigSchemaNode engine;
                                  engine.kind = ConfigSchemaKind::Object;
                                  engine.children.emplace("dry_run", makeBooleanNode());
                                  engine.children.emplace("enable_cache", makeBooleanNode());
                                  return engine;
                              }());

        root.children.emplace("environment", makeStringMapNode());
        return root;
    }();

    return Root;
}

[[nodiscard]] std::vector<std::string_view> splitDottedPath(std::string_view path)
{
    std::vector<std::string_view> segments;
    while (!path.empty())
    {
        const auto Dot = path.find('.');
        const auto Segment = path.substr(0, Dot);
        if (!Segment.empty())
        {
            segments.push_back(Segment);
        }

        if (Dot == std::string_view::npos)
        {
            break;
        }

        path.remove_prefix(Dot + 1);
    }

    return segments;
}

[[nodiscard]] const ConfigSchemaNode*
resolveSchemaNode(const ConfigSchemaNode& root, const std::vector<std::string_view>& segments)
{
    const ConfigSchemaNode* current = &root;
    for (const auto Segment : segments)
    {
        if (current->kind == ConfigSchemaKind::StringMap)
        {
            return nullptr;
        }

        const auto Child = current->children.find(std::string(Segment));
        if (Child == current->children.end())
        {
            return nullptr;
        }

        current = &Child->second;
    }

    return current;
}

[[nodiscard]] std::string displayPath(const std::string& dottedPath)
{
    return dottedPath.empty() ? "config" : dottedPath;
}

[[nodiscard]] std::string kindLabel(ConfigSchemaKind kind)
{
    switch (kind)
    {
    case ConfigSchemaKind::Object:
        return "table";
    case ConfigSchemaKind::Enum:
        return "enum";
    case ConfigSchemaKind::Boolean:
        return "boolean";
    case ConfigSchemaKind::Number:
        return "number";
    case ConfigSchemaKind::String:
        return "string";
    case ConfigSchemaKind::Path:
        return "path";
    case ConfigSchemaKind::StringMap:
        return "map";
    }

    return "unknown";
}

[[nodiscard]] std::string formatNumberConstraints(const NumberConstraints& constraints)
{
    std::ostringstream stream;
    stream << constraints.storage;
    if (constraints.min.has_value())
    {
        stream << ", >= " << *constraints.min;
    }
    if (constraints.max.has_value())
    {
        stream << ", <= " << *constraints.max;
    }
    if (constraints.defaultValue.has_value())
    {
        stream << ", default " << *constraints.defaultValue;
    }

    return stream.str();
}

[[nodiscard]] std::string childDetails(const ConfigSchemaNode& node)
{
    switch (node.kind)
    {
    case ConfigSchemaKind::Object:
        return std::to_string(node.children.size()) + " keys";
    case ConfigSchemaKind::Enum:
        return std::to_string(node.enumValues.size()) + " values";
    case ConfigSchemaKind::Boolean:
        return "true | false";
    case ConfigSchemaKind::Number:
        if (node.number.has_value())
        {
            return formatNumberConstraints(*node.number);
        }
        return {};
    case ConfigSchemaKind::String:
        return "free-form string";
    case ConfigSchemaKind::Path:
        return "filesystem path";
    case ConfigSchemaKind::StringMap:
        return "string keys, string values";
    }

    return {};
}

void appendHeader(std::ostringstream& stream, const std::string& dottedPath, ConfigSchemaKind kind)
{
    stream << "=== " << displayPath(dottedPath) << " ===\n";
    stream << "Kind: " << kindLabel(kind) << '\n';
}

void appendObjectListing(std::ostringstream& stream, const ConfigSchemaNode& node)
{
    if (node.children.empty())
    {
        return;
    }

    TextTable table({"Key", "Kind", "Details"});
    for (const auto& [key, child] : node.children)
    {
        table.addRow({key, kindLabel(child.kind), childDetails(child)});
    }

    stream << '\n' << table.format();
}

void appendEnumValues(std::ostringstream& stream, const ConfigSchemaNode& node)
{
    stream << "\nValues:\n";
    for (const auto& value : node.enumValues)
    {
        stream << "  " << value << '\n';
    }
}

void appendScalarDetails(std::ostringstream& stream, const ConfigSchemaNode& node)
{
    stream << '\n';
    switch (node.kind)
    {
    case ConfigSchemaKind::Boolean:
        stream << "Values: true, false\n";
        stream << "Default: true\n";
        break;
    case ConfigSchemaKind::Number:
        if (node.number.has_value())
        {
            stream << "Type: " << node.number->storage << '\n';
            if (node.number->min.has_value() || node.number->max.has_value())
            {
                stream << "Range:";
                if (node.number->min.has_value())
                {
                    stream << " >= " << *node.number->min;
                }
                if (node.number->max.has_value())
                {
                    stream << " <= " << *node.number->max;
                }
                stream << '\n';
            }
            if (node.number->defaultValue.has_value())
            {
                stream << "Default: " << *node.number->defaultValue << '\n';
            }
        }
        break;
    case ConfigSchemaKind::String:
        stream << "Format: free-form string\n";
        break;
    case ConfigSchemaKind::Path:
        stream << "Format: filesystem path (relative to project root or absolute)\n";
        break;
    case ConfigSchemaKind::StringMap:
        stream << "Keys: arbitrary string names\n";
        stream << "Values: string\n";
        break;
    case ConfigSchemaKind::Object:
    case ConfigSchemaKind::Enum:
        break;
    }
}

[[nodiscard]] std::string formatNodeOptions(const std::string& dottedPath,
                                            const ConfigSchemaNode& node)
{
    std::ostringstream stream;
    appendHeader(stream, dottedPath, node.kind);

    switch (node.kind)
    {
    case ConfigSchemaKind::Object:
        appendObjectListing(stream, node);
        break;
    case ConfigSchemaKind::Enum:
        appendEnumValues(stream, node);
        break;
    case ConfigSchemaKind::Boolean:
    case ConfigSchemaKind::Number:
    case ConfigSchemaKind::String:
    case ConfigSchemaKind::Path:
    case ConfigSchemaKind::StringMap:
        appendScalarDetails(stream, node);
        break;
    }

    std::string output = stream.str();
    if (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }

    return output;
}

}  // namespace

std::optional<std::string> formatConfigOptions(const std::string& dottedPath)
{
    const auto Segments = splitDottedPath(dottedPath);
    const ConfigSchemaNode* node = resolveSchemaNode(rootConfigSchema(), Segments);
    if (node == nullptr)
    {
        return std::nullopt;
    }

    return formatNodeOptions(dottedPath, *node);
}

}  // namespace beez::core
