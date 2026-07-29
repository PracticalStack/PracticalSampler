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
        const auto runtimeLoader = readText(root / "engine_adapter/src/RuntimeLoader.cpp");
        const auto playbackSnapshot = readText(root / "engine_adapter/src/PlaybackSnapshot.cpp");
        const auto authoringSession = readText(root / "engine_adapter/src/AuthoringSession.cpp");
        const auto authoringPanel = readText(root / "app/src/shared/AuthoringPanel.cpp");
        const auto zoneMappingEditor = readText(root / "app/src/shared/authoring/ZoneMappingEditor.cpp");

        const std::vector<std::pair<std::string, bool>> openGaps {
            { "snapshot group routes are still synthesized from zone.groupId membership only",
              playbackSnapshot.find("const auto groupIterator = groupRouteIndices.find(zone.groupId);")
                      != std::string::npos
                  || playbackSnapshot.find("routeObject[\"gainDb\"]") == std::string::npos },
            { "routing input selection still has no groups/<groupId> source option",
              authoringPanel.find("groups/") == std::string::npos },
            { "Round Robin editing still lives on zone surfaces instead of group surfaces",
              zoneMappingEditor.find("Round Robin") != std::string::npos
                  && authoringPanel.find("createRoundRobinPoolForSelectedZone") != std::string::npos },
            { "Round Robin compatibility is still the zone-local exact-match helper",
              authoringSession.find("return anchor.groupId == candidate.groupId") != std::string::npos
                  && authoringSession.find(
                         "sameVelocityCrossfadeDescriptor(anchor.velocityCrossfade, candidate.velocityCrossfade)")
                         != std::string::npos
                  && authoringSession.find("anchor.triggerMode == candidate.triggerMode")
                         != std::string::npos },
            { "there is still no creator-facing group manager or group inspector surface",
              authoringPanel.find("authoringGroup") == std::string::npos }
        };

        auto openGapCount = std::size_t { 0 };
        for (const auto& [label, observed] : openGaps)
        {
            if (observed)
            {
                ++openGapCount;
                std::cerr << "Phase 1 Zone Groups open gap: " << label << std::endl;
            }
        }

        if (openGapCount == 0)
        {
            std::cout << "Phase 1 Zone Groups Sprint 1 open-gap audit passed." << std::endl;
            return 0;
        }

        std::cerr << "Phase 1 Zone Groups Sprint 1 open-gap audit found " << openGapCount
                  << " remaining implementation gap(s)." << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 Zone Groups Sprint 1 open-gap audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
