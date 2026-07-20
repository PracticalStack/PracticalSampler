#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
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
} // namespace

int main()
{
    try
    {
        const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
        const auto facadeHeader = readText(root / "engine_adapter/include/drs/engine/EngineFacade.h");
        const auto facadeSource = readText(root / "engine_adapter/src/EngineFacade.cpp");
        const auto processorHeader = readText(root / "app/src/plugin/PluginProcessor.h");
        const auto statusPanel = readText(root / "app/src/shared/StatusPanel.cpp");
        const auto pluginEditor = readText(root / "app/src/plugin/PluginEditor.cpp");
        const auto standalone = readText(root / "app/src/standalone/MainComponent.cpp");

        const std::vector<std::pair<std::string, bool>> unresolved {
            { "StatusPanel directly invokes EngineFacade::publishCurrentDraft",
              statusPanel.find("engineFacade.publishCurrentDraft();") != std::string::npos },
            { "plug-in editor directly invokes EngineFacade::publishCurrentDraft",
              pluginEditor.find("owner.getEngineFacade().publishCurrentDraft();") != std::string::npos },
            { "standalone shell directly invokes EngineFacade::publishCurrentDraft",
              standalone.find("processor.getEngineFacade().publishCurrentDraft();") != std::string::npos },
            { "facade Publish implementation bypasses the typed controller",
              facadeSource.find("performancePublishController.request(") == std::string::npos },
            { "processor owns Performance activation eligibility and staging",
              processorHeader.find("bool stagePerformanceActivation(") != std::string::npos },
            { "published lifecycle remains string-only in the public facade snapshot",
              facadeHeader.find("PerformancePublishPresentationState publishedPresentationState")
                == std::string::npos },
            { "mutable facade macro values are not yet bound to an immutable published schema",
              facadeSource.find("buildPublishedMacroBindingTable(") == std::string::npos
                || processorHeader.find("activePublishedMacroCallbackView") == std::string::npos }
        };

        auto observedGapCount = std::size_t { 0 };
        for (const auto& [label, observed] : unresolved)
        {
            if (observed)
            {
                ++observedGapCount;
                std::cerr << "Sprint 6 Publish expected-red seam: " << label << std::endl;
            }
        }

        if (observedGapCount == 0)
        {
            std::cout << "Sprint 6 Publish replacement-seam audit passed." << std::endl;
            return 0;
        }

        std::cerr << "Sprint 6 Publish seam audit found " << observedGapCount
                  << " expected replacement seams." << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 6 Publish seam audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
