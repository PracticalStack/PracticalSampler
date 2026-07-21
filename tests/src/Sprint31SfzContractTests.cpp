#include "drs/engine/SfzImportContract.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const SfzImportCommand analyzeCommand;
        require(analyzeCommand.type == SfzImportCommandType::analyzeDocument,
                "Sprint 3.1.1 must start with an analyze-first SFZ command.");
        require(!sfzImportCommandRequiresReviewedReport(analyzeCommand.type),
                "Analyze-first SFZ import must not require a prior reviewed report.");
        require(sfzImportCommandRequiresReviewedReport(SfzImportCommandType::commitReviewedImport),
                "Final SFZ import commit must require a reviewed report.");

        require(isSfzImportStageTransitionAllowed(SfzImportStage::idle, SfzImportStage::discovering)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::discovering, SfzImportStage::parsing)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::parsing, SfzImportStage::normalizing)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::normalizing, SfzImportStage::validating)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::validating, SfzImportStage::classifying)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::classifying, SfzImportStage::projected)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::projected, SfzImportStage::reviewReady)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::reviewReady, SfzImportStage::committed),
                "The ordinary SFZ analysis and review pipeline must remain executable.");

        require(isSfzImportStageTransitionAllowed(SfzImportStage::discovering, SfzImportStage::blocked)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::parsing, SfzImportStage::canceled)
                    && isSfzImportStageTransitionAllowed(SfzImportStage::reviewReady, SfzImportStage::discovering),
                "SFZ analysis must support blocking, cancelation, and explicit re-analysis.");

        require(!isSfzImportStageTransitionAllowed(SfzImportStage::idle, SfzImportStage::reviewReady)
                    && !isSfzImportStageTransitionAllowed(SfzImportStage::parsing, SfzImportStage::committed)
                    && !isSfzImportStageTransitionAllowed(SfzImportStage::blocked, SfzImportStage::committed),
                "SFZ import must not skip analysis stages or commit from a blocked state.");

        const std::vector<SfzImportFinding> convertedOnly {
            { SfzImportFindingSeverity::information,
              SfzImportSupportDisposition::converted,
              "mapping.zone.converted",
              "Converted",
              "Zone mapping converted successfully.",
              { "fixture.sfz", 10, 1, SfzOpcodeScope::region, "sample" } }
        };
        require(sfzImportReviewDispositionFor(convertedOnly) == SfzImportReviewDisposition::noneRequired,
                "Converted informational findings should not force confirmation.");

        const std::vector<SfzImportFinding> approximated {
            { SfzImportFindingSeverity::warning,
              SfzImportSupportDisposition::approximated,
              "velocity.crossfade.lossy",
              "Approximation",
              "Velocity crossfade was approximated and must be reviewed.",
              { "fixture.sfz", 20, 1, SfzOpcodeScope::group, "xfin_lovel" } }
        };
        require(sfzImportReviewDispositionFor(approximated)
                    == SfzImportReviewDisposition::confirmationRequired,
                "Lossy or reported SFZ findings must require confirmation before commit.");

        const std::vector<SfzImportFinding> blocking {
            { SfzImportFindingSeverity::error,
              SfzImportSupportDisposition::blocking,
              "sample.missing",
              "Missing sample",
              "Referenced sample file could not be resolved.",
              { "fixture.sfz", 42, 1, SfzOpcodeScope::region, "sample" } }
        };
        require(sfzImportReviewDispositionFor(blocking) == SfzImportReviewDisposition::blocked,
                "Blocking SFZ findings must prevent final import commit.");

        require(SfzFirstFixtureCharacterization::expectedRegionCount == 225
                    && SfzFirstFixtureCharacterization::expectedGroupHeaderCount == 5
                    && SfzFirstFixtureCharacterization::expectedRoundRobinDepth == 3
                    && SfzFirstFixtureCharacterization::expectedVelocityLayerCount == 5,
                "The first checked-in SFZ fixture characterization changed unexpectedly.");

        std::cout << "Sprint 3.1.1 SFZ contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.1 SFZ contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
