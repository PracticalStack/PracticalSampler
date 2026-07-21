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
        const auto mainComponent = readText(root / "app/src/standalone/MainComponent.cpp");
        const auto pluginEditor = readText(root / "app/src/plugin/PluginEditor.cpp");

        const std::vector<std::pair<std::string, bool>> openGaps {
            { "standalone shell still has no .sfz chooser or review entry path",
              mainComponent.find(".sfz") == std::string::npos },
            { "plug-in shell still has no .sfz chooser or review entry path",
              pluginEditor.find(".sfz") == std::string::npos }
        };

        auto openGapCount = std::size_t { 0 };
        for (const auto& [label, observed] : openGaps)
        {
            if (observed)
            {
                ++openGapCount;
                std::cerr << "Sprint 3.1.5 SFZ open gap: " << label << std::endl;
            }
        }

        if (openGapCount == 0)
        {
            std::cout << "Sprint 3.1.5 SFZ open-gap audit passed." << std::endl;
            return 0;
        }

        std::cerr << "Sprint 3.1.5 SFZ open-gap audit found " << openGapCount
                  << " remaining implementation gap(s)." << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.5 SFZ open-gap audit failed unexpectedly: "
                  << exception.what() << std::endl;
        return 2;
    }
}
