#include "drs/engine/EngineFacade.h"
#include "shared/SfzImportReportModel.h"

#include <filesystem>
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
                "The shared SFZ report model headline changed unexpectedly.");
        require(model.guidance.find("velocity crossfades will be preserved") != std::string::npos,
                "The shared SFZ report model should publish creator guidance.");
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
