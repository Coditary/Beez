#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>

// NOLINTBEGIN(misc-include-cleaner)
#include <fcntl.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

namespace
{

constexpr std::size_t MaxInputSize = 65536U;

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

std::filesystem::path fuzzProjectRoot()
{
    static const std::filesystem::path Path =
        std::filesystem::temp_directory_path() / "beez_fuzz_project";
    std::filesystem::create_directories(Path);
    return Path;
}

std::filesystem::path fuzzScriptPath()
{
    return fuzzProjectRoot() / "build.lua";
}

bool writeFuzzInput(const uint8_t* data, const std::size_t InputSize)
{
    std::ofstream stream(fuzzScriptPath(), std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        return false;
    }

    stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(InputSize));
    return stream.good();
}

void loadFuzzInput()
{
    const StderrSilencer SilenceExpectedParseErrors;

    const beez::core::Context FuzzContext(fuzzProjectRoot());
    beez::plugin::lua::LuaDslLoader loader;
    {
        beez::core::Registry registry;
        static_cast<void>(loader.load(FuzzContext, registry));
    }
    loader.releaseState();
}

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming,readability-non-const-parameter)
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    (void)argc;
    (void)argv;
    // Belt-and-suspenders: disable LSan when the fuzzer binary is linked with ASan.
    // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c,concurrency-mt-unsafe,misc-include-cleaner)
    setenv("ASAN_OPTIONS", "detect_leaks=0", 1);
    return 0;
}

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size == 0 || size > MaxInputSize)
    {
        return 0;
    }

    if (!writeFuzzInput(data, size))
    {
        return 0;
    }

    loadFuzzInput();
    return 0;
}
