#include "drs/engine/SfzImportProjection.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path resolveFixture()
{
    const auto relative = fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr")
        / "jRhodes3d-mono/_jRhodes3d-mono-flac.sfz";
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);
    const auto candidates = { sourceRoot / relative,
                              sourceRoot.parent_path() / relative };
    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
            return candidate;
    }

    throw std::runtime_error("SFZ cancellation tests require the checked-in fixture.");
}

drs::engine::RuntimeProjectModel makeProject(const fs::path& fixture)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.runtimeProject";
    project.schemaVersion = 2;
    project.projectId = "sfz-cancellation-test";
    project.displayName = "SFZ cancellation test";
    project.contentRootPath = fixture.parent_path().generic_string();
    project.authoring.schemaName = "drs.authoringState";
    project.authoring.schemaVersion = 2;
    return project;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixture = resolveFixture();

        // A cancellation probe is polled from the recursive parser. The
        // threshold deliberately allows discovery to start, then guarantees
        // the result cannot be mistaken for a partial successful parse.
        std::size_t parserPolls = 0;
        SfzImportExecutionContext parserContext;
        parserContext.cancellationProbe = [&]
        {
            return ++parserPolls >= 4;
        };
        const auto canceledParse = parseSfzDocument(fixture.generic_string(), parserContext);
        require(canceledParse.execution.canceled()
                    && canceledParse.execution.cancellationReason == SfzImportCancellationReason::requested
                    && !canceledParse.complete
                    && !canceledParse.parsed,
                "Parser cancellation must publish a typed, unsuccessful terminal result.");

        SfzImportExecutionContext throwingContext;
        throwingContext.cancellationProbe = []() -> bool
        {
            throw std::runtime_error("probe failure");
        };
        const auto probeFailure = parseSfzDocument(fixture.generic_string(), throwingContext);
        require(probeFailure.execution.canceled()
                    && probeFailure.execution.cancellationReason
                        == SfzImportCancellationReason::probeException,
                "A throwing cancellation probe must become a typed safe cancellation.");

        // The progress seam is stage-weighted and monotonic. Cancel after
        // normalization reports its completed stage so report classification
        // cannot expose partial findings as a review-ready result.
        std::vector<SfzImportProgress> progress;
        auto cancelAfterNormalization = false;
        SfzImportExecutionContext reportContext;
        reportContext.progressEventSink = [&](const SfzImportProgress& event)
        {
            progress.push_back(event);
            if (event.stage == SfzImportStage::normalizing && event.progress01 >= 0.5f)
                cancelAfterNormalization = true;
        };
        reportContext.cancellationProbe = [&] { return cancelAfterNormalization; };
        const auto canceledAnalysis = analyzeSfzImportDocument(fixture.generic_string(), reportContext);
        require(canceledAnalysis.execution.canceled()
                    && canceledAnalysis.report.stage == SfzImportStage::canceled
                    && !canceledAnalysis.report.available,
                "Analysis cancellation must stop before report publication.");
        const auto monotonicProgress = std::adjacent_find(
                                           progress.begin(),
                                           progress.end(),
                                           [](const auto& left, const auto& right)
                                           {
                                               return left.progress01 > right.progress01;
                                           })
            == progress.end();
        require(!progress.empty() && monotonicProgress,
                "Progress events must be monotonic even when analysis is canceled.");

        // Projection checks at each source/zone boundary and clears all
        // partial vectors before returning its typed canceled outcome.
        const auto analysis = analyzeSfzImportDocument(fixture.generic_string());
        SfzImportExecutionContext projectionContext;
        projectionContext.cancellationProbe = [] { return true; };
        const auto canceledProjection = projectSfzImportAnalysis(makeProject(fixture),
                                                                   analysis,
                                                                   projectionContext);
        require(canceledProjection.execution.canceled()
                    && canceledProjection.execution.cancellationReason
                        == SfzImportCancellationReason::requested
                    && !canceledProjection.projected
                    && canceledProjection.zones.empty()
                    && canceledProjection.sampleSources.empty(),
                "Projection cancellation must release partial native content.");

        // The default overload remains the established synchronous path.
        const auto successful = analyzeSfzImportDocument(fixture.generic_string());
        require(successful.execution.completed()
                    && successful.report.stage == SfzImportStage::reviewReady,
                "The default analysis overload must retain the successful review path.");

        std::cout << "SFZ import cancellation/progress tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SFZ import cancellation/progress tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
