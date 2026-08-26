#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string>

namespace
{
bool containsAll(const std::string& text, std::initializer_list<const char*> needles)
{
    for (const auto* needle : needles)
    {
        if (text.find(needle) == std::string::npos)
            return false;
    }

    return true;
}
} // namespace

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
        const auto sfzLifecycleBegin = text.find("::reviewSfzImportFile(");
        const auto sfzLifecycleEnd = text.find("::showPreferencesDialog(", sfzLifecycleBegin);
        if (sfzLifecycleBegin == std::string::npos || sfzLifecycleEnd == std::string::npos)
        {
            std::cerr << "unable to isolate SFZ lifecycle in " << file << '\n';
            return 1;
        }
        const auto sfzLifecycle = text.substr(sfzLifecycleBegin, sfzLifecycleEnd - sfzLifecycleBegin);
        if (sfzLifecycle.find(".detach()") != std::string::npos
            || sfzLifecycle.find("MessageManager::callAsync") != std::string::npos)
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
        if (!containsAll(text,
                         {
                             "lastSfzImportDirectoryPropertyKey",
                             "getLastSfzImportDirectory()",
                             "setLastSfzImportDirectory(selectedFile.getParentDirectory())",
                             "settings->saveIfNeeded()"
                         }))
        {
            std::cerr << "persisted SFZ import directory integration missing in " << file << '\n';
            return 1;
        }
    }
    return 0;
}
