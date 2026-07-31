#include <filesystem>
#include <fstream>
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

bool containsAny(const std::string& text, std::initializer_list<const char*> needles)
{
    for (const auto* needle : needles)
    {
        if (text.find(needle) != std::string::npos)
            return true;
    }

    return false;
}
} // namespace

int main()
{
    const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
    const auto shellFiles = { root / "app/src/plugin/PluginEditor.cpp",
                              root / "app/src/standalone/MainComponent.cpp" };
    for (const auto& file : shellFiles)
    {
        std::ifstream input(file);
        if (!input)
        {
            std::cerr << "unable to read " << file << '\n';
            return 1;
        }

        const std::string text((std::istreambuf_iterator<char>(input)), {});
        if (!containsAll(text,
                         {
                             "getWavImportService().openClient()",
                             "wavImportProgress.setCancelCallback(",
                             "wavImportProgress.update(*snapshot);",
                             "WavImportRequest request;",
                             "wavImportClient->ownerId()",
                             "snapshot->identity.generation",
                             "snapshot->identity.contentRootPath",
                             "snapshot->identity.selectedGroupId",
                             "prepareWavImportBatchFromCompletion(",
                             "resolvePreparedWavImportManualRoot(",
                             "takePreparedWavImportCommit(",
                             "finalizePreparedWavImportCommit(",
                             "rollbackPreparedWavImportCommit(",
                             "pollWavImportService()"
                         }))
        {
            std::cerr << "async WAV import shell wiring drifted in " << file << '\n';
            return 1;
        }

        if (containsAny(text,
                        {
                            "prepareWavImportBatch(",
                            "createAuthoringImportQueue",
                            "processNextAuthoringImportQueueItem",
                            "copySampleFileForImport"
                        }))
        {
            std::cerr << "shell-specific WAV import implementation leaked back into " << file << '\n';
            return 1;
        }
    }

    const auto workflowFile = root / "app/src/shared/WavImportWorkflow.cpp";
    std::ifstream workflowInput(workflowFile);
    if (!workflowInput)
    {
        std::cerr << "unable to read " << workflowFile << '\n';
        return 1;
    }

    const std::string workflowText((std::istreambuf_iterator<char>(workflowInput)), {});
    if (!containsAll(workflowText,
                     {
                         "prepareWavImportBatchFromCompletion(",
                         "resolvePreparedWavImportManualRoot(",
                         "takePreparedWavImportCommit(",
                         "finalizePreparedWavImportCommit(",
                         "rollbackPreparedWavImportCommit("
                     }))
    {
        std::cerr << "shared WAV import workflow drifted away from the async completion contract\n";
        return 1;
    }

    if (containsAny(workflowText,
                    {
                        "prepareWavImportBatch(",
                        "createAuthoringImportQueue",
                        "processNextAuthoringImportQueueItem",
                        "copySampleFileForImport",
                        "chooseUniqueSampleDestination"
                    }))
    {
        std::cerr << "legacy synchronous WAV import workflow leaked back into shared product code\n";
        return 1;
    }

    return 0;
}
