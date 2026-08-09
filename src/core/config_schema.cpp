#include "beez/core/config_schema.hpp"

#include "beez/core/cache_options.hpp"
#include "beez/core/performance_options.hpp"
#include "beez/core/ui_options.hpp"
#include "beez/core/util/text_table.hpp"
#include "beez/logging/settings/logging_settings.hpp"

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

        root.children.emplace(
            "ui",
            []()
            {
                ConfigSchemaNode uiSettings;
                uiSettings.kind = ConfigSchemaKind::Object;
                uiSettings.children.emplace("output_mode",
                                            makeEnumNode({"clean", "verbose", "errors", "silent"}));
                uiSettings.children.emplace("colors", makeBooleanNode());
                uiSettings.children.emplace("truecolor", makeBooleanNode());
                uiSettings.children.emplace("theme", makeStringNode());
                uiSettings.children.emplace("themes", makeStringMapNode());
                uiSettings.children.emplace("icons", makeBooleanNode());

                ConfigSchemaNode animation;
                animation.kind = ConfigSchemaKind::Object;
                animation.children.emplace("progress", makeEnumNode(progressDisplayStyleNames()));
                animation.children.emplace("indicator",
                                           makeEnumNode(progressIndicatorStyleNames()));
                animation.children.emplace("indicator_spin_interval",
                                           makeNumberNode(NumberConstraints {}));
                uiSettings.children.emplace("animation", std::move(animation));

                uiSettings.children.emplace("log_level", makeEnumNode(uiLogLevelNames()));
                uiSettings.children.emplace("hide_cache_hits", makeBooleanNode());
                uiSettings.children.emplace("prefix", makeBooleanNode());
                uiSettings.children.emplace("prefix_format", makeStringNode());
                uiSettings.children.emplace("show_time_saved", makeBooleanNode());
                uiSettings.children.emplace("summary", makeEnumNode(runSummaryStyleNames()));

                ConfigSchemaNode logging;
                logging.kind = ConfigSchemaKind::Object;
                logging.children.emplace("run_log", makeBooleanNode());
                logging.children.emplace("run_log_file", makePathNode());
                logging.children.emplace("log_steps", makeBooleanNode());
                logging.children.emplace("workers", makeEnumNode(logging::workerLogModeNames()));
                logging.children.emplace("workers_dir", makePathNode());
                uiSettings.children.emplace("logging", std::move(logging));

                return uiSettings;
            }());

        root.children.emplace("env",
                              []()
                              {
                                  ConfigSchemaNode env;
                                  env.kind = ConfigSchemaKind::Object;
                                  env.children.emplace("load_dotenv", makeBooleanNode());
                                  env.children.emplace("dotenv_overrides_system",
                                                       makeBooleanNode());
                                  env.children.emplace("files", makePathNode());
                                  env.children.emplace("vars", makeStringMapNode());
                                  env.children.emplace("hash_vars", makeStringNode());
                                  env.children.emplace("ignore_vars_for_hashing", makeStringNode());
                                  env.children.emplace("mask_secrets", makeStringNode());
                                  return env;
                              }());
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
    if (node.enumValues.empty())
    {
        return;
    }

    TextTable table({"Value"});
    for (const auto& value : node.enumValues)
    {
        table.addRow({value});
    }

    stream << '\n' << table.format();
}

void appendScalarDetails(std::ostringstream& stream, const ConfigSchemaNode& node)
{
    std::vector<std::pair<std::string, std::string>> rows;

    switch (node.kind)
    {
    case ConfigSchemaKind::Boolean:
        rows.emplace_back("Allowed", "true, false");
        rows.emplace_back("Default", "true");
        break;
    case ConfigSchemaKind::Number:
        if (node.number.has_value())
        {
            rows.emplace_back("Type", node.number->storage);
            if (node.number->min.has_value() || node.number->max.has_value())
            {
                std::ostringstream range;
                if (node.number->min.has_value())
                {
                    range << ">= " << *node.number->min;
                }
                if (node.number->max.has_value())
                {
                    if (!range.str().empty())
                    {
                        range << ", ";
                    }
                    range << "<= " << *node.number->max;
                }
                rows.emplace_back("Range", range.str());
            }
            if (node.number->defaultValue.has_value())
            {
                rows.emplace_back("Default", *node.number->defaultValue);
            }
        }
        break;
    case ConfigSchemaKind::String:
        rows.emplace_back("Format", "free-form string");
        break;
    case ConfigSchemaKind::Path:
        rows.emplace_back("Format", "filesystem path (relative to project root or absolute)");
        break;
    case ConfigSchemaKind::StringMap:
        rows.emplace_back("Keys", "arbitrary string names");
        rows.emplace_back("Values", "string");
        break;
    case ConfigSchemaKind::Object:
    case ConfigSchemaKind::Enum:
        break;
    }

    if (rows.empty())
    {
        return;
    }

    TextTable table({"Property", "Value"});
    for (const auto& [property, value] : rows)
    {
        table.addRow({property, value});
    }

    stream << '\n' << table.format();
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

[[nodiscard]] std::vector<std::string> listObjectChildPaths(const ConfigSchemaNode& node,
                                                            const std::string& basePath)
{
    std::vector<std::string> completions;
    completions.reserve(node.children.size());
    for (const auto& [key, child] : node.children)
    {
        (void)child;
        completions.push_back(basePath + key);
    }

    std::ranges::sort(completions);
    return completions;
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

std::vector<std::string> listConfigOptionCompletions(const std::string& prefix)
{
    const ConfigSchemaNode& root = rootConfigSchema();

    if (prefix.empty())
    {
        return listObjectChildPaths(root, "");
    }

    if (prefix.ends_with('.'))
    {
        const std::string ParentPath = prefix.substr(0, prefix.size() - 1);
        const ConfigSchemaNode* node = resolveSchemaNode(root, splitDottedPath(ParentPath));
        if (node != nullptr && node->kind == ConfigSchemaKind::Object)
        {
            return listObjectChildPaths(*node, prefix);
        }
        return {};
    }

    const ConfigSchemaNode* exactNode = resolveSchemaNode(root, splitDottedPath(prefix));
    if (exactNode != nullptr && exactNode->kind == ConfigSchemaKind::Object)
    {
        return listObjectChildPaths(*exactNode, prefix + '.');
    }

    std::string_view parentPath;
    std::string_view partial;
    const auto LastDot = prefix.rfind('.');
    if (LastDot == std::string::npos)
    {
        parentPath = {};
        partial = prefix;
    }
    else
    {
        parentPath = std::string_view(prefix).substr(0, LastDot);
        partial = std::string_view(prefix).substr(LastDot + 1);
    }

    const ConfigSchemaNode* node = resolveSchemaNode(root, splitDottedPath(parentPath));
    if (node == nullptr || node->kind != ConfigSchemaKind::Object)
    {
        return {};
    }

    const std::string Base = parentPath.empty() ? std::string {} : std::string(parentPath) + '.';

    std::vector<std::string> completions;
    completions.reserve(node->children.size());
    for (const auto& [key, child] : node->children)
    {
        (void)child;
        if (!partial.empty() && !key.starts_with(partial))
        {
            continue;
        }

        completions.push_back(Base + key);
    }

    std::ranges::sort(completions);
    return completions;
}

}  // namespace beez::core
