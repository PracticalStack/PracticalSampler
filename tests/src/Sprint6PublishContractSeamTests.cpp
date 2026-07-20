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
} // namespace

int main()
{
    try
    {
        const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
        const auto facadeHeader = readText(root / "engine_adapter/include/drs/engine/EngineFacade.h");
        const auto facadeSource = readText(root / "engine_adapter/src/EngineFacade.cpp");
        const auto controllerHeader = readText(
            root / "engine_adapter/include/drs/engine/PerformancePublishController.h");
        const auto processorHeader = readText(root / "app/src/plugin/PluginProcessor.h");
        const auto processorSource = readText(root / "app/src/plugin/PluginProcessor.cpp");
        const auto statusPanel = readText(root / "app/src/shared/StatusPanel.cpp");
        const auto pluginEditor = readText(root / "app/src/plugin/PluginEditor.cpp");
        const auto standalone = readText(root / "app/src/standalone/MainComponent.cpp");

        const std::vector<std::pair<std::string, bool>> violations {
            { "StatusPanel directly invokes EngineFacade::publishCurrentDraft",
              statusPanel.find("engineFacade.publishCurrentDraft();") != std::string::npos },
            { "plug-in editor directly invokes EngineFacade::publishCurrentDraft",
              pluginEditor.find("owner.getEngineFacade().publishCurrentDraft();") != std::string::npos },
            { "standalone shell directly invokes EngineFacade::publishCurrentDraft",
              standalone.find("processor.getEngineFacade().publishCurrentDraft();") != std::string::npos },
            { "facade Publish implementation bypasses the typed controller",
              facadeSource.find("performancePublishController.request(") == std::string::npos },
            { "processor owns obsolete Performance eligibility staging",
              processorHeader.find("bool stagePerformanceActivation(") != std::string::npos },
            { "public facade retains the compatibility publishedRevisionState string",
              facadeHeader.find("publishedRevisionState") != std::string::npos },
            { "facade retains duplicate active published macro binding state",
              facadeHeader.find("activePublishedMacroBindings") != std::string::npos },
            { "controller does not own the active immutable publication payload",
              controllerHeader.find("getActiveActivationPayload()") == std::string::npos },
            { "processor lacks the shared typed Publish adapter",
              processorHeader.find("PerformancePublishCommandAdapter performancePublishCommandAdapter")
                  == std::string::npos
                || processorSource.find("submitPerformancePublishCommand(") == std::string::npos },
            { "processor lacks the bounded realtime published macro callback view",
              processorHeader.find("activePublishedMacroCallbackView") == std::string::npos }
        };

        auto violationCount = std::size_t { 0 };
        for (const auto& [label, observed] : violations)
        {
            if (observed)
            {
                ++violationCount;
                std::cerr << "Sprint 6 Publish seam violation: " << label << std::endl;
            }
        }

        if (violationCount != 0)
        {
            std::cerr << "Sprint 6 Publish seam audit found " << violationCount
                      << " regression(s)." << std::endl;
            return 1;
        }

        std::cout << "Sprint 6 Publish replacement-seam audit passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 6 Publish seam audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
