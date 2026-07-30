#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
    const auto files = { root / "app/src/plugin/PluginEditor.cpp",
                         root / "app/src/standalone/MainComponent.cpp" };
    for (const auto& file : files)
    {
        std::ifstream input(file);
        if (!input)
        {
            std::cerr << "unable to read " << file << '\n';
            return 1;
        }
        const std::string text((std::istreambuf_iterator<char>(input)), {});
        if (text.find(".detach()") != std::string::npos
            || text.find("MessageManager::callAsync") != std::string::npos)
        {
            std::cerr << "detached SFZ shell worker remains in " << file << '\n';
            return 1;
        }
        if (text.find("getSfzImportReviewService") == std::string::npos
            || text.find("pollSfzImportReviewService") == std::string::npos
            || text.find("Project changed") == std::string::npos)
        {
            std::cerr << "service/stale-guard integration missing in " << file << '\n';
            return 1;
        }
    }
    return 0;
}
