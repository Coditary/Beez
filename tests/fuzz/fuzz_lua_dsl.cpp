#include "beez/core/context.h"
#include "beez/core/registry.h"
#include "beez/plugin/lua/lua_dsl.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#include <fcntl.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

namespace
{

constexpr std::size_t kMaxInputSize = 64 * 1024;

class StderrSilencer
{
  public:
    StderrSilencer()
    {
        std::fflush(stderr);
        saved_ = dup(STDERR_FILENO);
        const int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0)
        {
            dup2(nullFd, STDERR_FILENO);
            close(nullFd);
        }
    }

    ~StderrSilencer()
    {
        if (saved_ >= 0)
        {
            std::fflush(stderr);
            dup2(saved_, STDERR_FILENO);
            close(saved_);
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

bool writeFuzzInput(const uint8_t* data, const std::size_t size)
{
    std::ofstream stream(fuzzScriptPath(), std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        return false;
    }

    stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return stream.good();
}

void loadFuzzInput()
{
    const StderrSilencer silenceExpectedParseErrors;

    beez::core::Context context(fuzzProjectRoot());
    beez::plugin::lua::LuaDslLoader loader;
    {
        beez::core::Registry registry;
        static_cast<void>(loader.load(context, registry));
    }
    loader.releaseState();
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    (void)argc;
    (void)argv;
    // Belt-and-suspenders: disable LSan when the fuzzer binary is linked with ASan.
    setenv("ASAN_OPTIONS", "detect_leaks=0", 1);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const std::size_t size)
{
    if (data == nullptr || size == 0 || size > kMaxInputSize)
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
