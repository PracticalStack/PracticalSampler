#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not read " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

std::filesystem::path resolveStudioRoot()
{
    const auto sourceRoot = std::filesystem::path(DRS_SOURCE_ROOT);
    if (std::filesystem::exists(sourceRoot / "engine_adapter"))
        return sourceRoot;

    const auto nestedStudioRoot = sourceRoot / "DecentRhapsodyStudio";
    if (std::filesystem::exists(nestedStudioRoot / "engine_adapter"))
        return nestedStudioRoot;

    throw std::runtime_error("Could not locate the DecentRhapsodyStudio source root.");
}
} // namespace

int main()
{
    try
    {
        const auto root = resolveStudioRoot();
        const auto runtimeModel = readText(root / "engine_adapter/include/drs/engine/RuntimeModel.h");
        const auto runtimeVoice = readText(root / "engine_adapter/src/RuntimeVoice.cpp");
        const auto sampleImportHeader = readText(root / "engine_adapter/include/drs/engine/SampleImport.h");
        const auto authoringPanel = readText(root / "app/src/shared/AuthoringPanel.cpp");
        const auto zoneMappingEditor = readText(root / "app/src/shared/authoring/ZoneMappingEditor.cpp");

        const std::vector<std::pair<std::string, bool>> openGaps {
            { "native runtime model still has no explicit round-robin pool identity field",
              runtimeModel.find("poolId") == std::string::npos },
            { "native runtime model still has no explicit round-robin mode field",
              runtimeModel.find("roundRobinMode") == std::string::npos },
            { "runtime route resolution still depends on voice-id modulo selection",
              runtimeVoice.find("% static_cast<std::uint64_t>(roundRobinLength)") != std::string::npos },
            { "sample import suggestions still expose only a flat roundRobinIndex seam",
              sampleImportHeader.find("roundRobinIndex") != std::string::npos },
            { "authoring UI still has no explicit Round Robin editor surface",
              authoringPanel.find("Round Robin") == std::string::npos
                  && zoneMappingEditor.find("Round Robin") == std::string::npos }
        };

        auto openGapCount = std::size_t { 0 };
        for (const auto& [label, observed] : openGaps)
        {
            if (observed)
            {
                ++openGapCount;
                std::cerr << "Sprint 3.1.3.1 Round Robin open gap: " << label << std::endl;
            }
        }

        if (openGapCount == 0)
        {
            std::cout << "Sprint 3.1.3.1 Round Robin open-gap audit passed." << std::endl;
            return 0;
        }

        std::cerr << "Sprint 3.1.3.1 Round Robin open-gap audit found " << openGapCount
                  << " remaining implementation gap(s)." << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.3.1 Round Robin open-gap audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
