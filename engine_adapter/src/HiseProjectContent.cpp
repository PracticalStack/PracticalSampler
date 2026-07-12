#include "drs/engine/HiseProjectContent.h"

#include "drs/engine/WorkspacePaths.generated.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

struct DirectorySpec
{
    const char* name;
    const char* relativePath;
    std::array<const char*, 6> extensions;
};

constexpr std::array<DirectorySpec, 9> directorySpecs {{
    {"Images", "Images", {".png", ".jpg", ".jpeg", ".svg", ".gif", ".webp"}},
    {"SampleMaps", "SampleMaps", {".xml", ".json", "", "", "", ""}},
    {"UserPresets", "UserPresets", {".xml", ".preset", ".hip", "", "", ""}},
    {"AudioFiles", "AudioFiles", {".wav", ".aif", ".aiff", ".flac", ".ogg", ".mp3"}},
    {"Samples", "Samples", {".wav", ".aif", ".aiff", ".flac", ".ogg", ".ncw"}},
    {"Expansions", "Expansions", {".xml", ".json", ".hr1", "", "", ""}},
    {"Scripts", "Scripts", {".js", "", "", "", "", ""}},
    {"DspNetworks", "DspNetworks", {".xml", ".json", ".h", ".hpp", "", ""}},
    {"XmlPresetBackups", "XmlPresetBackups", {".xml", "", "", "", "", ""}}
}};

bool hasMatchingExtension(const fs::path& filePath, const DirectorySpec& spec)
{
    const auto extension = filePath.extension().generic_string();

    for (const auto* allowed : spec.extensions)
    {
        if (allowed[0] == '\0')
            continue;

        if (extension == allowed)
            return true;
    }

    return false;
}

std::size_t countMatchingFiles(const fs::path& directoryPath, const DirectorySpec& spec)
{
    std::error_code errorCode;

    if (!fs::exists(directoryPath, errorCode))
        return 0;

    std::size_t count = 0;
    fs::recursive_directory_iterator iterator(directoryPath, fs::directory_options::skip_permission_denied, errorCode);
    const fs::recursive_directory_iterator end;

    while (!errorCode && iterator != end)
    {
        if (iterator->is_regular_file(errorCode) && hasMatchingExtension(iterator->path(), spec))
            ++count;

        iterator.increment(errorCode);
    }

    return count;
}

std::string toDisplayPath(const fs::path& path)
{
    return path.generic_string();
}

std::vector<HiseProjectDirectorySnapshot> buildDirectorySnapshots(const fs::path& rootPath)
{
    std::vector<HiseProjectDirectorySnapshot> directories;
    directories.reserve(directorySpecs.size());

    for (const auto& spec : directorySpecs)
    {
        const auto absolutePath = rootPath / spec.relativePath;
        std::error_code errorCode;

        directories.push_back({
            spec.name,
            spec.relativePath,
            toDisplayPath(absolutePath),
            fs::exists(absolutePath, errorCode),
            countMatchingFiles(absolutePath, spec)
        });
    }

    return directories;
}

std::string getEnvVarOrEmpty(const char* name)
{
    if (const auto* value = std::getenv(name))
        return value;

    return {};
}

fs::path getRuntimeAppDataRoot()
{
    const auto appData = getEnvVarOrEmpty("APPDATA");

    if (appData.empty())
        return {};

    return fs::path(appData)
        / generated::runtimeCompanyName
        / generated::runtimeProductName;
}
} // namespace

HiseProjectContentSnapshot getHiseProjectContentSnapshot()
{
    const fs::path repoRoot(generated::workspaceRepoRoot);
    const fs::path repoContentRoot(generated::workspaceHiseProjectRoot);
    const fs::path runtimeAppDataRoot = getRuntimeAppDataRoot();

    std::error_code errorCode;
    auto repoDirectories = buildDirectorySnapshots(repoContentRoot);
    auto runtimeDirectories = buildDirectorySnapshots(runtimeAppDataRoot);

    std::size_t presetFileCount = 0;
    std::size_t sampleMapFileCount = 0;

    for (const auto& directory : repoDirectories)
    {
        if (directory.name == "UserPresets")
            presetFileCount = directory.matchingFileCount;
        else if (directory.name == "SampleMaps")
            sampleMapFileCount = directory.matchingFileCount;
    }

    return {
        fs::exists(repoContentRoot, errorCode),
        toDisplayPath(repoRoot),
        toDisplayPath(repoContentRoot),
        toDisplayPath(runtimeAppDataRoot),
        std::move(repoDirectories),
        std::move(runtimeDirectories),
        presetFileCount,
        sampleMapFileCount
    };
}
} // namespace drs::engine
