#include "beez/core/reqpack/format.hpp"
#include "beez/core/reqpack/types.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <yyjson.h>
// NOLINTEND(misc-include-cleaner)

namespace beez::core
{

namespace
{

[[nodiscard]] bool isMetadataKey(std::string_view key)
{
    return key == "ok" || key == "dryRun";
}

[[nodiscard]] std::optional<std::string> optionalStringField(yyjson_val* object, const char* field)
{
    const char* value = yyjson_get_str(yyjson_obj_get(object, field));
    if (value == nullptr)
    {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] ReqPackPackageResult parsePackageResultObject(yyjson_val* object)
{
    const char* name = yyjson_get_str(yyjson_obj_get(object, "name"));
    if (name == nullptr)
    {
        throw std::runtime_error("rqp json package entry missing string name");
    }

    ReqPackPackageResult package {.name = name};
    package.version = optionalStringField(object, "version");
    package.status = optionalStringField(object, "status");
    package.errorMessage = optionalStringField(object, "errorMessage");
    return package;
}

[[nodiscard]] std::vector<ReqPackPackageResult> parsePackageResultArray(yyjson_val* array)
{
    if (!yyjson_is_arr(array))
    {
        throw std::runtime_error("rqp json plugin value must be an array");
    }

    std::vector<ReqPackPackageResult> packages;
    yyjson_arr_iter iterator = yyjson_arr_iter_with(array);
    yyjson_val* entry = nullptr;
    while ((entry = yyjson_arr_iter_next(&iterator)) != nullptr)
    {
        if (!yyjson_is_obj(entry))
        {
            throw std::runtime_error("rqp json package entry must be an object");
        }
        packages.push_back(parsePackageResultObject(entry));
    }
    return packages;
}

}  // namespace

std::string formatInstallArg(const std::string& plugin, const ReqPackPackage& package)
{
    std::string arg = plugin + ':' + package.name;
    if (package.version.has_value())
    {
        arg += '@';
        arg += *package.version;
    }
    return arg;
}

std::vector<std::string> buildInstallArgs(const ReqPackManifest& manifest)
{
    std::vector<std::string> args;
    for (const auto& [plugin, packages] : manifest.plugins)
    {
        std::ranges::transform(packages,
                               std::back_inserter(args),
                               [&](const ReqPackPackage& package)
                               { return formatInstallArg(plugin, package); });
    }
    return args;
}

std::string buildInstallCommand(const std::vector<std::string>& args, bool dryRun)
{
    std::ostringstream stream;
    stream << "rqp install";
    for (const auto& arg : args)
    {
        stream << ' ' << arg;
    }
    if (dryRun)
    {
        stream << " --dry-run";
    }
    stream << " --json";
    return stream.str();
}

ReqPackInstallResponse parseRqpJsonResponse(const std::string& json)
{
    const std::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)> JsonDocument(
        yyjson_read(json.data(), json.size(), YYJSON_READ_NOFLAG), &yyjson_doc_free);
    if (JsonDocument == nullptr)
    {
        throw std::runtime_error("failed to parse rqp json output");
    }

    yyjson_val* root = yyjson_doc_get_root(JsonDocument.get());
    if (!yyjson_is_obj(root))
    {
        throw std::runtime_error("rqp json root must be an object");
    }

    ReqPackInstallResponse response;
    if (yyjson_val* okValue = yyjson_obj_get(root, "ok"); okValue != nullptr)
    {
        if (!yyjson_is_bool(okValue))
        {
            throw std::runtime_error("rqp json ok field must be a boolean");
        }
        response.ok = yyjson_get_bool(okValue);
    }

    if (yyjson_val* dryRunValue = yyjson_obj_get(root, "dryRun"); dryRunValue != nullptr)
    {
        if (!yyjson_is_bool(dryRunValue))
        {
            throw std::runtime_error("rqp json dryRun field must be a boolean");
        }
        response.dryRun = yyjson_get_bool(dryRunValue);
    }

    yyjson_obj_iter iterator = yyjson_obj_iter_with(root);
    yyjson_val* key = nullptr;
    yyjson_val* value = nullptr;
    while ((key = yyjson_obj_iter_next(&iterator)) != nullptr)
    {
        const char* plugin = yyjson_get_str(key);
        if (plugin == nullptr)
        {
            continue;
        }

        if (isMetadataKey(plugin))
        {
            continue;
        }

        value = yyjson_obj_iter_get_val(key);
        response.plugins.emplace(plugin, parsePackageResultArray(value));
    }

    return response;
}

std::string formatRqpInstallErrors(const ReqPackInstallResponse& response)
{
    std::ostringstream stream;
    stream << "reqpack install failed:\n";

    bool wroteFailure = false;
    for (const auto& [plugin, packages] : response.plugins)
    {
        for (const auto& package : packages)
        {
            if (!package.failed())
            {
                continue;
            }

            wroteFailure = true;
            stream << "  " << plugin << ':' << package.name;
            if (package.version.has_value() && !package.version->empty())
            {
                stream << '@' << *package.version;
            }
            if (package.errorMessage.has_value() && !package.errorMessage->empty())
            {
                stream << ": " << *package.errorMessage;
            }
            stream << '\n';
        }
    }

    if (!wroteFailure && response.ok.has_value() && !*response.ok)
    {
        stream << "  rqp reported ok=false without package-level failure details\n";
    }

    return stream.str();
}

}  // namespace beez::core
