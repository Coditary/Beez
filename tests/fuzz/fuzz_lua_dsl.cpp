#include "beez/core/registry/registry.hpp"
#include "beez/core/runtime/context.hpp"
#include "beez/core/util/temp_directory.hpp"
#include "beez/plugin/lua/lua_dsl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner)
#include <fcntl.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

// NOLINTBEGIN(readability-identifier-naming,modernize-avoid-c-arrays,readability-avoid-nested-conditional-operator,bugprone-empty-catch,misc-include-cleaner)
namespace
{

constexpr std::size_t MaxInputSize = 65536U;
constexpr std::uint8_t EnvScriptSeparator = 0x1EU;
constexpr std::uint8_t ScriptSuffixSeparator = 0x1FU;

constexpr std::uint8_t ModeDoubleLoad = 0x01U;
constexpr std::uint8_t ModeFreshLoader = 0x02U;
constexpr std::uint8_t ModeValidateConsistent = 0x04U;
constexpr std::uint8_t ModeSetProcessEnv = 0x08U;
constexpr std::uint8_t ModeWriteJunkFile = 0x10U;
constexpr std::uint8_t ModeGcThroughput = 0x20U;
constexpr std::uint8_t ModeTripleLoad = 0x40U;

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerMutate(uint8_t* data, std::size_t size, std::size_t maxSize);

class StderrSilencer
{
  public:
    StderrSilencer()
    {
        static_cast<void>(std::fflush(stderr));
        saved_ = dup(STDERR_FILENO);
        const int NullFd = open("/dev/null", O_WRONLY);
        if (NullFd >= 0)
        {
            static_cast<void>(dup2(NullFd, STDERR_FILENO));
            static_cast<void>(close(NullFd));
        }
    }

    ~StderrSilencer()
    {
        if (saved_ >= 0)
        {
            static_cast<void>(std::fflush(stderr));
            static_cast<void>(dup2(saved_, STDERR_FILENO));
            static_cast<void>(close(saved_));
        }
    }

    StderrSilencer(const StderrSilencer&) = delete;
    StderrSilencer& operator=(const StderrSilencer&) = delete;

  private:
    int saved_ = -1;
};

class FuzzProcessEnvGuard
{
  public:
    void set(const char* name, const char* value)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
        static_cast<void>(setenv(name, value, 1));
        names_.emplace_back(name);
    }

    ~FuzzProcessEnvGuard()
    {
        for (const auto& name : names_)
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe,cert-env33-c)
            static_cast<void>(unsetenv(name.c_str()));
        }
    }

  private:
    std::vector<std::string> names_;
};

std::filesystem::path fuzzProjectRoot()
{
    static const std::filesystem::path Path =
        beez::core::systemTempDirectory() / "beez_fuzz_project";
    std::filesystem::create_directories(Path);
    return Path;
}

std::filesystem::path fuzzScriptPath()
{
    return fuzzProjectRoot() / "build.lua";
}

std::filesystem::path fuzzEnvPath()
{
    return fuzzProjectRoot() / ".env";
}

std::filesystem::path fuzzJunkPath()
{
    return fuzzProjectRoot() / "beez_fuzz_junk";
}

struct FuzzSections
{
    std::uint8_t mode = 0U;
    const uint8_t* envData = nullptr;
    std::size_t envSize = 0U;
    const uint8_t* scriptData = nullptr;
    std::size_t scriptSize = 0U;
    const uint8_t* suffixData = nullptr;
    std::size_t suffixSize = 0U;
};

FuzzSections parseFuzzInput(const uint8_t* data, const std::size_t size)
{
    FuzzSections sections {};
    if (size < 2U)
    {
        sections.scriptData = data;
        sections.scriptSize = size;
        return sections;
    }

    sections.mode = data[0];
    const uint8_t* payload = data + 1U;
    std::size_t payloadSize = size - 1U;

    std::size_t envEnd = payloadSize;
    for (std::size_t index = 0; index < payloadSize; ++index)
    {
        if (payload[index] == EnvScriptSeparator)
        {
            sections.envData = payload;
            sections.envSize = index;
            payload = payload + index + 1U;
            payloadSize = payloadSize - index - 1U;
            envEnd = index;
            break;
        }
    }
    (void)envEnd;

    for (std::size_t index = 0; index < payloadSize; ++index)
    {
        if (payload[index] == ScriptSuffixSeparator)
        {
            sections.scriptData = payload;
            sections.scriptSize = index;
            sections.suffixData = payload + index + 1U;
            sections.suffixSize = payloadSize - index - 1U;
            return sections;
        }
    }

    sections.scriptData = payload;
    sections.scriptSize = payloadSize;
    return sections;
}

bool writeBinaryFile(const std::filesystem::path& path, const uint8_t* data, const std::size_t size)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        return false;
    }

    if (size > 0U)
    {
        stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    return stream.good();
}

bool writeFuzzEnv(const uint8_t* data, const std::size_t size)
{
    if (size == 0U)
    {
        std::error_code error;
        std::filesystem::remove(fuzzEnvPath(), error);
        return true;
    }

    return writeBinaryFile(fuzzEnvPath(), data, size);
}

bool writeFuzzScript(const std::vector<uint8_t>& script)
{
    return writeBinaryFile(fuzzScriptPath(), script.data(), script.size());
}

void applyFuzzProcessEnv(const FuzzSections& sections, FuzzProcessEnvGuard& guard)
{
    if ((sections.mode & ModeSetProcessEnv) == 0U)
    {
        return;
    }

    char valueBuffer[32];
    for (std::size_t index = 0; index < 8U; ++index)
    {
        char nameBuffer[24];
        // NOLINTNEXTLINE(cert-err33-c)
        static_cast<void>(std::snprintf(nameBuffer, sizeof(nameBuffer), "BEEZ_FZ_%zu", index));
        const uint8_t valueByte =
            sections.envSize > index
                ? sections.envData[index]
                : (sections.scriptSize > index ? sections.scriptData[index]
                                               : static_cast<uint8_t>(sections.mode + index));
        // NOLINTNEXTLINE(cert-err33-c)
        static_cast<void>(std::snprintf(valueBuffer, sizeof(valueBuffer), "%02x", valueByte));
        guard.set(nameBuffer, valueBuffer);
    }
}

void touchRegistryOnSuccess(const beez::core::Registry& registry,
                            beez::plugin::lua::LuaDslLoader& loader,
                            const std::uint8_t mode)
{
    static_cast<void>(loader.buildSettings());

    if ((mode & ModeValidateConsistent) != 0U)
    {
        try
        {
            registry.validateConsistent();
        }
        catch (...)
        {
        }
    }

    for (const auto& [name, task] : registry.tasks())
    {
        static_cast<void>(name);
        static_cast<void>(task.actions.size());
    }

    for (const auto& [name, step] : registry.steps())
    {
        static_cast<void>(name);
        static_cast<void>(step.phase);
    }

    for (const auto& [name, workflow] : registry.workflows())
    {
        static_cast<void>(name);
        static_cast<void>(workflow.steps.size());
    }
}

bool loadOnce(beez::plugin::lua::LuaDslLoader& loader,
              const beez::core::Context& context,
              const std::uint8_t mode)
{
    beez::core::Registry registry;
    const bool Loaded = loader.load(context, registry);
    if (Loaded)
    {
        if ((mode & ModeGcThroughput) != 0U)
        {
            loader.setGcThroughputMode(true);
            loader.setGcThroughputMode(false);
        }

        touchRegistryOnSuccess(registry, loader, mode);
    }

    return Loaded;
}

void loadFuzzInput(const FuzzSections& sections)
{
    const StderrSilencer SilenceExpectedParseErrors;
    FuzzProcessEnvGuard envGuard;
    applyFuzzProcessEnv(sections, envGuard);

    const beez::core::Context FuzzContext(fuzzProjectRoot());
    beez::plugin::lua::LuaDslLoader loader;
    static_cast<void>(loadOnce(loader, FuzzContext, sections.mode));
    static_cast<void>(loadOnce(loader, FuzzContext, sections.mode));

    if ((sections.mode & ModeTripleLoad) != 0U)
    {
        static_cast<void>(loadOnce(loader, FuzzContext, sections.mode));
    }

    if ((sections.mode & ModeDoubleLoad) != 0U)
    {
        static_cast<void>(loadOnce(loader, FuzzContext, sections.mode));
    }

    beez::plugin::lua::LuaDslLoader freshLoader;
    static_cast<void>(loadOnce(freshLoader, FuzzContext, sections.mode));

    if ((sections.mode & ModeFreshLoader) != 0U)
    {
        beez::plugin::lua::LuaDslLoader secondFreshLoader;
        static_cast<void>(loadOnce(secondFreshLoader, FuzzContext, sections.mode));
        secondFreshLoader.releaseState();
    }

    freshLoader.releaseState();
    loader.releaseState();
}

std::vector<uint8_t> assembleScript(const FuzzSections& sections)
{
    std::vector<uint8_t> script;
    script.reserve(sections.scriptSize + sections.suffixSize);
    script.insert(script.end(), sections.scriptData, sections.scriptData + sections.scriptSize);
    if (sections.suffixSize > 0U)
    {
        script.insert(script.end(), sections.suffixData, sections.suffixData + sections.suffixSize);
    }

    return script;
}

constexpr std::array<const char*, 48> DslFragments = {
    "task(",
    "workflow(",
    "step({",
    "beez.config({",
    "configure_step(",
    "order(",
    "beez.env(",
    "name = ",
    "phase = ",
    "scope = ",
    "run = ",
    "parallel = ",
    "config = ",
    "input = ",
    "output = ",
    "mutate = ",
    "description = ",
    "\"true\"",
    "\"false\"",
    "\"\"",
    "{}",
    "{ }",
    "[]",
    "nil",
    "42",
    "-1",
    "0x1e",
    "0x1f",
    "\n",
    "\r\n",
    "-- fuzz\n",
    "[[",
    "]]",
    "function",
    "end",
    "local ",
    "return ",
    "table.insert",
    "string.rep",
    "for i = 1, ",
    " do ",
    " end",
    "performance",
    "cache",
    "env",
    "ui",
    "duplicate",
    "missing",
};

std::size_t insertFragment(uint8_t* data, std::size_t size, std::size_t maxSize, unsigned int seed)
{
    if (size >= maxSize)
    {
        return size;
    }

    const char* fragment = DslFragments[seed % DslFragments.size()];
    const std::size_t fragmentSize = std::strlen(fragment);
    if (fragmentSize == 0U || size + fragmentSize > maxSize)
    {
        return size;
    }

    const std::size_t insertAt = (size == 0U) ? 0U : seed % size;
    if (insertAt < size)
    {
        static_cast<void>(
            std::memmove(data + insertAt + fragmentSize, data + insertAt, size - insertAt));
    }

    std::copy_n(fragment, fragmentSize, data + insertAt);
    return size + fragmentSize;
}

std::size_t duplicateSlice(uint8_t* data, std::size_t size, std::size_t maxSize, unsigned int seed)
{
    if (size < 2U || size >= maxSize)
    {
        return size;
    }

    const std::size_t sliceStart = seed % (size / 2U);
    const std::size_t sliceLen = 1U + ((seed / 7U) % (size - sliceStart));
    if (size + sliceLen > maxSize)
    {
        return size;
    }

    std::vector<uint8_t> slice(data + sliceStart, data + sliceStart + sliceLen);
    static_cast<void>(
        std::memmove(data + sliceStart + sliceLen, data + sliceStart, size - sliceStart));
    static_cast<void>(std::memcpy(data + sliceStart, slice.data(), sliceLen));
    return size + sliceLen;
}

std::size_t injectSeparator(uint8_t* data, std::size_t size, std::size_t maxSize, unsigned int seed)
{
    if (size >= maxSize)
    {
        return size;
    }

    const std::uint8_t separator = (seed % 2U) == 0U ? EnvScriptSeparator : ScriptSuffixSeparator;
    const std::size_t insertAt = seed % (size + 1U);
    if (insertAt < size)
    {
        static_cast<void>(std::memmove(data + insertAt + 1U, data + insertAt, size - insertAt));
    }

    data[insertAt] = separator;
    return size + 1U;
}

}  // namespace
// NOLINTEND(readability-identifier-naming,modernize-avoid-c-arrays,readability-avoid-nested-conditional-operator,bugprone-empty-catch,misc-include-cleaner)

// NOLINTBEGIN(readability-identifier-naming,readability-non-const-parameter)
// NOLINTNEXTLINE(readability-identifier-naming,readability-non-const-parameter)
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    (void)argc;
    (void)argv;
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe,misc-include-cleaner)
    setenv("ASAN_OPTIONS", "detect_leaks=0", 1);
    return 0;
}

extern "C" size_t
LLVMFuzzerCustomMutator(uint8_t* data, size_t size, size_t maxSize, unsigned int seed)
{
    if (data == nullptr || maxSize == 0U)
    {
        return 0U;
    }

    const unsigned int Mutation = seed % 10U;
    if (Mutation == 0U)
    {
        size = insertFragment(data, size, maxSize, seed);
    }
    else if (Mutation == 1U)
    {
        size = injectSeparator(data, size, maxSize, seed);
    }
    else if (Mutation == 2U)
    {
        size = duplicateSlice(data, size, maxSize, seed);
    }

    return static_cast<size_t>(LLVMFuzzerMutate(data, size, maxSize));
}

extern "C" size_t LLVMFuzzerCustomCrossOver(const uint8_t* data1,
                                            size_t size1,
                                            const uint8_t* data2,
                                            size_t size2,
                                            uint8_t* out,
                                            size_t maxOut,
                                            unsigned int seed)
{
    if (out == nullptr || maxOut == 0U || data1 == nullptr || data2 == nullptr)
    {
        return 0U;
    }

    const FuzzSections sections1 = parseFuzzInput(data1, size1);
    const FuzzSections sections2 = parseFuzzInput(data2, size2);

    std::vector<uint8_t> merged;
    merged.reserve(maxOut);
    merged.push_back(static_cast<uint8_t>((sections1.mode ^ sections2.mode) | ModeDoubleLoad));

    const bool swapParents = (seed % 2U) == 0U;
    const auto& envParent = swapParents ? sections2 : sections1;
    const auto& scriptParent = swapParents ? sections1 : sections2;

    if (envParent.envSize > 0U)
    {
        merged.insert(merged.end(), envParent.envData, envParent.envData + envParent.envSize);
        merged.push_back(EnvScriptSeparator);
    }

    if (scriptParent.scriptSize > 0U)
    {
        merged.insert(merged.end(),
                      scriptParent.scriptData,
                      scriptParent.scriptData + scriptParent.scriptSize);
    }

    if (scriptParent.suffixSize > 0U && merged.size() + scriptParent.suffixSize + 1U <= maxOut)
    {
        merged.push_back(ScriptSuffixSeparator);
        merged.insert(merged.end(),
                      scriptParent.suffixData,
                      scriptParent.suffixData + scriptParent.suffixSize);
    }

    if (merged.size() > maxOut)
    {
        merged.resize(maxOut);
    }

    if (merged.empty())
    {
        return static_cast<size_t>(LLVMFuzzerMutate(out, 0U, maxOut));
    }

    static_cast<void>(std::memcpy(out, merged.data(), merged.size()));
    return merged.size();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size == 0 || size > MaxInputSize)
    {
        return 0;
    }

    const FuzzSections sections = parseFuzzInput(data, size);
    if (sections.scriptSize == 0U && sections.suffixSize == 0U)
    {
        return 0;
    }

    const std::vector<uint8_t> script = assembleScript(sections);
    if (!writeFuzzEnv(sections.envData, sections.envSize) || !writeFuzzScript(script))
    {
        return 0;
    }

    if ((sections.mode & ModeWriteJunkFile) != 0U)
    {
        static_cast<void>(
            writeBinaryFile(fuzzJunkPath(), data, std::min(size, std::size_t {1024U})));
    }
    else
    {
        std::error_code error;
        std::filesystem::remove(fuzzJunkPath(), error);
    }

    loadFuzzInput(sections);
    return 0;
}
// NOLINTEND(readability-identifier-naming,readability-non-const-parameter)
