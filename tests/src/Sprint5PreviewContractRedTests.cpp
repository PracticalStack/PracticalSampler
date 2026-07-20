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
        const auto processorSource = readText(root / "app/src/plugin/PluginProcessor.cpp");
        const auto processorHeader = readText(root / "app/src/plugin/PluginProcessor.h");
        const auto previewModel = readText(root / "app/src/shared/AuthoringPreviewModel.h");
        const auto noteOnStart = processorSource.find("void Processor::queueAuthoringPreviewNoteOn");
        const auto noteOnEnd = noteOnStart == std::string::npos
            ? std::string::npos
            : processorSource.find("\n}\n", noteOnStart);
        const auto noteOnBody = noteOnStart == std::string::npos
            ? std::string {}
            : processorSource.substr(noteOnStart, noteOnEnd - noteOnStart);

        const std::vector<std::pair<std::string, bool>> unresolved {
            { "processor-owned immediate Preview payload construction",
              processorSource.find("processor-preview-snapshot-") != std::string::npos },
            { "synchronous selected-sample warming in Preview staging",
              processorSource.find("ensureSelectedAuthoringSampleLoaded(false)") != std::string::npos },
            { "implicit message servicing from Preview note-on",
              noteOnBody.find("serviceMessageThreadWork();") != std::string::npos },
            { "processor-owned Preview lifecycle synchronization",
              processorHeader.find("synchronizeAuthoringPreviewActivation") != std::string::npos },
            { "string-only public Preview lifecycle state",
              previewModel.find("std::string revisionState") != std::string::npos }
        };

        auto observedGapCount = std::size_t { 0 };
        for (const auto& [label, observed] : unresolved)
        {
            if (observed)
            {
                ++observedGapCount;
                std::cerr << "Sprint 5 Preview seam regression: " << label << std::endl;
            }
        }

        if (observedGapCount == 0)
        {
            std::cout << "Sprint 5 Preview replacement-seam regression audit passed."
                      << std::endl;
            return 0;
        }

        std::cerr << "Sprint 5 Preview seam audit found " << observedGapCount
                  << " regressed replacement seams." << std::endl;
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 5 Preview seam audit failed unexpectedly: " << exception.what() << std::endl;
        return 2;
    }
}
