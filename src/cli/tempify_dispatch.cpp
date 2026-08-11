#include "beez/cli/tempify_dispatch.hpp"

#include "tempify/app/TempifyApp.h"
#include "tempify/build/ReapplySerialization.h"
#include "tempify/support/Errors.h"

#include <support/Diagnostic.h>

#include <exception>
#include <iostream>
#include <sstream>
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

    if (args[0] == "list" || args[0] == "info" || args[0] == "doctor" || args[0] == "validate" ||
        args[0] == "inspect" || args[0] == "lint" || args[0] == "refresh" || args[0] == "reapply" ||
        args[0] == "test")
    {
        for (std::size_t index = 1; index < args.size(); ++index)
        {
            if (args[index] == "--json")
            {
                return true;
            }
        }
    }

    if (args[0] != "list" && args[0] != "info" && args[0] != "doctor" && args[0] != "validate" &&
        args[0] != "inspect" && args[0] != "lint" && args[0] != "refresh" && args[0] != "test" &&
        args[0] != "completion" && args[0] != "process" && args[0] != "reapply" && args[0] != "-p" &&
        args[0] != "--prebyte" && args[0] != "help")
    {
        for (std::size_t index = 1; index < args.size(); ++index)
        {
            if (args[index] == "--json")
            {
                return true;
            }
        }
    }

    return false;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
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
            escaped.push_back(character);
            break;
        }
    }
    return escaped;
}

void writeJsonError(const std::string& code, const std::string& message)
{
    std::cerr << "{\n"
              << "  \"status\": \"error\",\n"
              << "  \"code\": \"" << code << "\",\n"
              << "  \"message\": \"" << jsonEscape(message) << "\"\n"
              << "}\n";
}

}  // namespace

bool isInitMode(int argc, const char* const* argv)
{
    if (argc < 2 || argv == nullptr)
    {
        return false;
    }

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
        const tempify::TempifyApp app;
        return app.run(args);
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
