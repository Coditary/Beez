#include "beez/version.hpp"

#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return 0;

    std::string input(reinterpret_cast<const char*>(data), size);

    if (input == "version")
    {
        volatile int v = beez::version::MajorVersion;
        (void)v;
    }

    return 0;
}
