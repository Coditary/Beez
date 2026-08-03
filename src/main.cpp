#include "beez/version.hpp"

#include <iostream>

int main()
{
    std::cout << "Beez v" << beez::version::MajorVersion << "." << beez::version::MinorVersion << "."
              << beez::version::PatchVersion << "\n";
    std::cout << "Hello, World!\n";
    return 0;
}
