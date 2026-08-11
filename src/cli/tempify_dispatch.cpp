#include "beez/cli/tempify_dispatch.hpp"

#include "tempify/app/TempifyApp.h"
#include "tempify/build/ReapplySerialization.h"
#include "tempify/support/Errors.h"

#include <support/Diagnostic.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace beez::cli
{

namespace
{

bool wantsJsonErrors(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        return false;
    }

    const std::string& command = args.front();

    if (command == "list" || command == "info" || command == "doctor" || command == "validate" ||
        command == "inspect" || command == "lint" || command == "refresh" || command == "reapply" ||
        command == "test")
    {
        return std::ranges::any_of(std::next(args.begin()),
                                   args.end(),
                                   [](const std::string& arg) { return arg == "--json"; });
    }

    if (command != "list" && command != "info" && command != "doctor" && command != "validate" &&
        command != "inspect" && command != "lint" && command != "refresh" && command != "test" &&
        command != "completion" && command != "process" && command != "reapply" &&
        command != "-p" && command != "--prebyte" && command != "help")
    {
        return std::ranges::any_of(std::next(args.begin()),
                                   args.end(),
                                   [](const std::string& arg) { return arg == "--json"; });
    }

    return false;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char Character : value)
    {
        switch (Character)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(Character);
            break;
        }
    }
    return escaped;
}

void writeJsonError(const std::string& code, const std::string& message)
{
    std::cerr << "{\n"
              << "  \"status\": \"error\",\n"
              << R"(  "code": ")" << code << R"(",)" << '\n'
              << R"(  "message": ")" << jsonEscape(message) << R"(")" << '\n'
              << "}\n";
}

}  // namespace

bool isInitMode(int argc, const char* const* argv)
{
    if (argc < 2 || argv == nullptr)
    {
        return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::string_view(argv[1]) == "--init";
}

std::vector<std::string> collectInitArgs(int argc, const char* const* argv)
{
    std::vector<std::string> args;
    if (argv == nullptr)
    {
        return args;
    }

    args.reserve(static_cast<std::size_t>(argc > 2 ? argc - 2 : 0));
    for (int index = 2; index < argc; ++index)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        args.emplace_back(argv[index]);
    }
    return args;
}

int runTempifyInitMode(const std::vector<std::string>& args)
{
    try
    {
        const tempify::TempifyApp App;
        return App.run(args);
    }
    catch (const tempify::ReapplyBlockedError& error)
    {
        if (wantsJsonErrors(args))
        {
            std::cerr << tempify::format_reapply_blocked_error_json(error);
        }
        else
        {
            std::cerr << error.what() << '\n';
        }
        return 1;
    }
    catch (const prebyte::DiagnosticError& error)
    {
        if (wantsJsonErrors(args))
        {
            writeJsonError("CLI_ERROR", error.what());
        }
        else
        {
            std::cerr << error.what() << '\n';
        }
        return 1;
    }
    catch (const std::exception& error)
    {
        if (wantsJsonErrors(args))
        {
            writeJsonError("CLI_ERROR", error.what());
        }
        else
        {
            std::cerr << error.what() << '\n';
        }
        return 1;
    }
}

}  // namespace beez::cli
