#pragma once

#include <string>
#include <vector>

namespace beez::core
{

struct CacheLookupResult
{
    bool skip = false;
    std::string key;
    double savedDurationSeconds = 0.0;
};

struct CacheEntry
{
    std::string key;
    std::string stepName;
    std::vector<std::string> outputs;
    // Content hashes parallel to outputs; entries without hashes are treated as stale.
    std::vector<std::string> outputHashes;
};

}  // namespace beez::core
