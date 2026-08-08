// NOLINTBEGIN(misc-include-cleaner,concurrency-mt-unsafe,performance-no-automatic-move,bugprone-command-processor,cert-env33-c)
#include "beez/cli/install_completion.hpp"
#include "beez/cli/completion_embedded.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

namespace beez::cli
{

namespace
{

[[nodiscard]] std::filesystem::path configEnvPath()
{
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME"); configHome != nullptr)
    {
        return std::filesystem::path(configHome) / "beez" / "config.env";
    }

    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        return std::filesystem::path(home) / ".config" / "beez" / "config.env";
    }

    return {};
}

[[nodiscard]] std::optional<std::filesystem::path>
readConfigValue(const std::filesystem::path& path, const std::string& key)
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        return std::nullopt;
    }

    const std::string Prefix = key + '=';
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.starts_with(Prefix))
        {
            std::string value = line.substr(Prefix.size());
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            {
                value = value.substr(1, value.size() - 2);
            }
            return value;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> rootFromExecutable(const char* argv0)
{
    if (argv0 == nullptr)
    {
        return std::nullopt;
    }

    std::error_code errorCode;
    const auto Executable = std::filesystem::weakly_canonical(argv0, errorCode);
    if (errorCode)
    {
        return std::nullopt;
    }

    const auto BinDir = Executable.parent_path();
    if (BinDir.filename() != "bin")
    {
        return std::nullopt;
    }

    const auto BuildTypeDir = BinDir.parent_path();
    const auto BuildDir = BuildTypeDir.parent_path();
    if (BuildDir.filename() != "build")
    {
        return std::nullopt;
    }

    const auto ProjectBuildDir = BuildDir.parent_path();
    if (ProjectBuildDir.filename() != "build")
    {
        return std::nullopt;
    }

    return ProjectBuildDir.parent_path();
}

[[nodiscard]] std::optional<std::filesystem::path> dataInstallScriptPath()
{
    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        const auto Script = std::filesystem::path(home) / ".local" / "share" / "beez" /
                            "install-beez-completion.sh";
        if (std::filesystem::exists(Script))
        {
            return Script;
        }
    }

    if (const char* dataHome = std::getenv("XDG_DATA_HOME"); dataHome != nullptr)
    {
        const auto Script = std::filesystem::path(dataHome) / "beez" / "install-beez-completion.sh";
        if (std::filesystem::exists(Script))
        {
            return Script;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> installScriptPath(const char* argv0)
{
    if (const auto DataScript = dataInstallScriptPath())
    {
        return DataScript;
    }

    const auto ConfigPath = configEnvPath();
    if (!ConfigPath.empty())
    {
        if (const auto Root = readConfigValue(ConfigPath, "BEEZ_ROOT_DIR"))
        {
            const auto Script = *Root / "scripts" / "install-beez-completion.sh";
            if (std::filesystem::exists(Script))
            {
                return Script;
            }
        }
    }

    if (const auto Root = rootFromExecutable(argv0))
    {
        const auto Script = *Root / "scripts" / "install-beez-completion.sh";
        if (std::filesystem::exists(Script))
        {
            return Script;
        }
    }

    return std::nullopt;
}

[[nodiscard]] int runShellCommand(const std::string& command)
{
    const int Status = std::system(command.c_str());
    if (Status == -1)
    {
        return 1;
    }

    if (!WIFEXITED(Status))
    {
        return 1;
    }

    return WEXITSTATUS(Status);
}

[[nodiscard]] bool writeEmbeddedCompletions(const std::filesystem::path& destination)
{
    std::error_code errorCode;
    std::filesystem::create_directories(destination, errorCode);
    if (errorCode)
    {
        return false;
    }

    for (const auto& file : embeddedCompletionFiles())
    {
        std::ofstream stream(destination / file.name);
        if (!stream.is_open())
        {
            return false;
        }

        stream << file.content;
        if (!stream.good())
        {
            return false;
        }
    }

    return true;
}

[[nodiscard]] int runInstallFromCompletionsDir(const std::filesystem::path& completionsSource)
{
    const auto InstallLib = completionsSource / "install-lib.sh";
    const std::string command = "bash -c 'source \"" + InstallLib.string() + "\" && " +
                                "install_beez_completion_files \"" + completionsSource.string() +
                                "\" && install_beez_completion_hooks'";
    return runShellCommand(command);
}

[[nodiscard]] int runEmbeddedInstallCompletion()
{
    const auto StagingDir =
        std::filesystem::temp_directory_path() / ("beez-completion-" + std::to_string(getpid()));

    std::error_code errorCode;
    std::filesystem::remove_all(StagingDir, errorCode);

    if (!writeEmbeddedCompletions(StagingDir))
    {
        std::cerr << "Error: failed to stage embedded shell completion files\n";
        return 1;
    }

    const int ExitCode = runInstallFromCompletionsDir(StagingDir);
    std::filesystem::remove_all(StagingDir, errorCode);
    return ExitCode;
}

}  // namespace

int runInstallCompletion(const char* argv0)
{
    if (const auto Script = installScriptPath(argv0))
    {
        return runShellCommand("bash " + Script->string());
    }

    return runEmbeddedInstallCompletion();
}

}  // namespace beez::cli
// NOLINTEND(misc-include-cleaner,concurrency-mt-unsafe,performance-no-automatic-move,bugprone-command-processor,cert-env33-c)
