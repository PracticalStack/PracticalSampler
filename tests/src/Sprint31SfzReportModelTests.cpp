#include "drs/engine/EngineFacade.h"
#include "shared/SfzImportReportModel.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::filesystem::path resolveFirstFixturePath()
{
    const auto sourceRoot = std::filesystem::path(DRS_SOURCE_ROOT);
    const auto relativeFixturePath =
        std::filesystem::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (std::filesystem::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (std::filesystem::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

std::filesystem::path resolveFirstSamplePath(const std::filesystem::path& fixturePath)
{
    constexpr std::array extensions { ".flac", ".wav", ".aif", ".aiff" };
    for (const auto& entry : std::filesystem::recursive_directory_iterator(fixturePath.parent_path()))
    {
        if (!entry.is_regular_file())
            continue;

        const auto extension = entry.path().extension().generic_string();
        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
            return entry.path();
    }

    throw std::runtime_error("Could not locate a sample asset next to the first SFZ fixture.");
}

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}
} // namespace

int main()
{
    try
    {
        drs::engine::EngineFacade engineFacade;
        const auto fixturePath = resolveFirstFixturePath();
        const auto analysis = engineFacade.analyzeSfzImportDocument(fixturePath.generic_string());
        const auto model = drs::app::makeSfzImportReportModel(analysis);

        require(model.available,
                "The shared SFZ report model should be available after facade analysis succeeds.");
        require(model.stage == drs::engine::SfzImportStage::reviewReady,
                "The shared SFZ report model should mirror the review-ready engine stage.");
        require(model.reviewDisposition
                    == drs::engine::SfzImportReviewDisposition::confirmationRequired,
                "The shared SFZ report model should require confirmation for the first fixture.");
        require(model.commitAllowed,
                "The shared SFZ report model should allow a future explicit commit after review.");
        require(model.confirmationRequired,
                "The shared SFZ report model should preserve the explicit confirmation gate.");
        require(model.headline == "Review SFZ import",
                "The shared SFZ report model headline changed unexpectedly: " + model.headline);
        require(model.guidance.find("velocity crossfades will be preserved") != std::string::npos,
                "The shared SFZ report model should publish creator guidance.");
        require(model.guidance.find("round robins will be grouped into native Round Robin pools") != std::string::npos,
                "The shared SFZ report model should acknowledge converted sequential round-robin pools.");
        require(model.guidance.find("master, group, and zone gains") != std::string::npos,
                "The shared SFZ report model should acknowledge scoped gain preservation.");
        require(model.documentPath == analysis.report.rootDocumentPath,
                "The shared SFZ report model should preserve the analyzed document path.");
        require(model.convertedCount == 1599
                    && model.approximatedCount == 0
                    && model.reportedOnlyCount == 9
                    && model.blockingCount == 0,
                "The shared SFZ report model summary counts changed unexpectedly.");
        require(model.report.findings.size() == 9,
                "The shared SFZ report model should preserve the full compatibility finding list.");

        const auto missingAnalysis = engineFacade.analyzeSfzImportDocument(
            (fixturePath.parent_path() / "_missing-sfz-fixture.sfz").generic_string());
        const auto missingModel = drs::app::makeSfzImportReportModel(missingAnalysis);
        require(missingModel.available
                    && missingModel.reviewDisposition
                        == drs::engine::SfzImportReviewDisposition::blocked
                    && !missingModel.commitAllowed
                    && missingModel.headline == "SFZ import blocked",
                "The shared SFZ report model should preserve blocked facade analysis results.");

        const auto samplePath = resolveFirstSamplePath(fixturePath).lexically_normal();
        const auto tempDirectory = std::filesystem::temp_directory_path() / "drs-sprint31-sfz-report-model";
        const auto scopedGainFixturePath = tempDirectory / "global-volume-review.sfz";
        writeTextFile(
            scopedGainFixturePath,
            "<global>\n"
            "volume=-1\n"
            "<master>\n"
            "volume=2\n"
            "<group>\n"
            "volume=-3\n"
            "<region>\n"
            "sample=" + samplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "volume=4\n");
        const auto scopedGainAnalysis = engineFacade.analyzeSfzImportDocument(scopedGainFixturePath.generic_string());
        const auto scopedGainModel = drs::app::makeSfzImportReportModel(scopedGainAnalysis);
        require(scopedGainModel.available
                    && scopedGainModel.confirmationRequired
                    && scopedGainModel.approximatedCount == 1,
                "The shared SFZ report model should surface scoped-gain approximations.");
        require(scopedGainModel.guidance.find("gain scope still needs review") != std::string::npos,
                "The shared SFZ report model should explain scoped-gain approximations explicitly.");

        std::cout << "Sprint 3.1.3 SFZ report-model tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.3 SFZ report-model tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
