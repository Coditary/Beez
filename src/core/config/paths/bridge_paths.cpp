#include "beez/core/config/paths/bridge_paths.hpp"

#include "beez/core/config/paths/config_paths.hpp"

#include <yyjson.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace beez::core
{

std::filesystem::path bridgeDirectory()
{
    return beezConfigDirectory() / "bridges";
}

std::filesystem::path bridgeIndexPath()
{
    return bridgeDirectory() / "index.json";
}

std::string hashPath(const std::filesystem::path& path)
{
    const auto PathString = path.string();
    const auto HashValue = std::hash<std::string>{}(PathString);
    std::ostringstream stream;
    stream << std::hex << HashValue;
    return stream.str();
}

namespace
{

struct BridgeIndex
{
    struct Entry
    {
        std::string sourcePath;
        std::string hash;
    };

    std::vector<Entry> entries;
};

[[nodiscard]] BridgeIndex readIndex(const std::filesystem::path& indexPath)
{
    BridgeIndex index;
    if (!std::filesystem::exists(indexPath))
    {
        return index;
    }

    std::ifstream file(indexPath);
    if (!file.is_open())
    {
        return index;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (content.empty())
    {
        return index;
    }

    std::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)> doc(
        yyjson_read(content.data(), content.size(), YYJSON_READ_NOFLAG), &yyjson_doc_free);
    if (doc == nullptr)
    {
        return index;
    }

    yyjson_val* root = yyjson_doc_get_root(doc.get());
    if (!yyjson_is_obj(root))
    {
        return index;
    }

    yyjson_obj_iter iter = yyjson_obj_iter_with(root);
    yyjson_val* key = nullptr;
    while ((key = yyjson_obj_iter_next(&iter)) != nullptr)
    {
        const char* sourcePath = yyjson_get_str(key);
        if (sourcePath == nullptr)
        {
            continue;
        }

        yyjson_val* value = yyjson_obj_iter_get_val(key);
        if (!yyjson_is_obj(value))
        {
            continue;
        }

        const char* hash = yyjson_get_str(yyjson_obj_get(value, "hash"));
        if (hash == nullptr)
        {
            continue;
        }

        index.entries.push_back({std::string(sourcePath), std::string(hash)});
    }

    return index;
}

[[nodiscard]] bool writeIndex(const std::filesystem::path& indexPath, const BridgeIndex& index)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
    if (doc == nullptr)
    {
        return false;
    }

    yyjson_mut_val* root = yyjson_mut_obj(doc);
    if (root == nullptr)
    {
        yyjson_mut_doc_free(doc);
        return false;
    }

    yyjson_mut_doc_set_root(doc, root);

    for (const auto& entry : index.entries)
    {
        yyjson_mut_val* entryObj = yyjson_mut_obj(doc);
        if (entryObj == nullptr)
        {
            continue;
        }

        yyjson_mut_val* hashVal = yyjson_mut_str(doc, entry.hash.c_str());
        if (hashVal == nullptr)
        {
            continue;
        }

        yyjson_mut_obj_add(entryObj, yyjson_mut_str(doc, "hash"), hashVal);
        yyjson_mut_obj_add(root,
                           yyjson_mut_str(doc, entry.sourcePath.c_str()),
                           entryObj);
    }

    yyjson_write_err writeErr = {};
    char* json = yyjson_mut_write_opts(
        doc,
        static_cast<yyjson_write_flag>(YYJSON_WRITE_PRETTY | YYJSON_WRITE_ALLOW_INVALID_UNICODE),
        nullptr,
        nullptr,
        &writeErr);
    yyjson_mut_doc_free(doc);

    if (json == nullptr)
    {
        return false;
    }

    std::ofstream outFile(indexPath);
    if (!outFile.is_open())
    {
        std::free(json);
        return false;
    }

    outFile << json;
    outFile.close();
    std::free(json);
    return true;
}

}  // namespace

std::optional<std::filesystem::path> resolveBridge(const std::filesystem::path& projectRoot)
{
    const auto AbsoluteProjectRoot = std::filesystem::weakly_canonical(projectRoot);
    const auto ProjectRootString = AbsoluteProjectRoot.string();
    const auto Index = readIndex(bridgeIndexPath());

    for (const auto& entry : Index.entries)
    {
        if (entry.sourcePath == ProjectRootString)
        {
            const auto BridgePath = bridgeDirectory() / entry.hash / "build.lua";
            if (std::filesystem::exists(BridgePath))
            {
                return BridgePath;
            }
        }
    }

    return std::nullopt;
}

BridgeLinkResult createBridgeLink(const std::filesystem::path& buildScriptSource,
                                  const std::filesystem::path& projectRoot)
{
    const auto AbsoluteProjectRoot = std::filesystem::weakly_canonical(projectRoot);
    const auto AbsoluteSource = std::filesystem::weakly_canonical(buildScriptSource);
    const auto ProjectRootString = AbsoluteProjectRoot.string();
    const auto Hash = hashPath(AbsoluteProjectRoot);
    const auto BridgeDir = bridgeDirectory() / Hash;
    const auto TargetPath = BridgeDir / "build.lua";

    const auto IndexPath = bridgeIndexPath();

    // Check if already exists in index
    auto index = readIndex(IndexPath);
    for (const auto& entry : index.entries)
    {
        if (entry.sourcePath == ProjectRootString)
        {
            const auto ExistingBridge = bridgeDirectory() / entry.hash / "build.lua";
            if (std::filesystem::exists(ExistingBridge))
            {
                return {.bridgeDir = bridgeDirectory() / entry.hash, .alreadyExisted = true};
            }
            // Entry exists but bridge is missing; remove stale entry
            break;
        }
    }

    // Create bridge directory
    std::filesystem::create_directories(BridgeDir);

    // Copy build.lua to bridge
    std::filesystem::copy_file(AbsoluteSource, TargetPath, std::filesystem::copy_options::overwrite_existing);

    // Remove stale entries for this project root
    std::erase_if(index.entries, [&](const BridgeIndex::Entry& entry)
    {
        return entry.sourcePath == ProjectRootString;
    });

    // Add new entry
    index.entries.push_back({AbsoluteProjectRoot.string(), Hash});

    // Write updated index
    std::filesystem::create_directories(bridgeDirectory());
    if (!writeIndex(IndexPath, index))
    {
        // Best effort - bridge dir and file are still created
    }

    return {.bridgeDir = BridgeDir, .alreadyExisted = false};
}

}  // namespace beez::core
