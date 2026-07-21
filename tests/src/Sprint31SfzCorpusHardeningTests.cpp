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

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path resolveFixturePath(const fs::path& relativeFixturePath)
{
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (fs::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (fs::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

drs::engine::RuntimeProjectModel makeBlankPhase2Project(const fs::path& fixturePath,
                                                        const std::string& projectId)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = projectId;
    project.displayName = "Sprint 3.1.6 Corpus Hardening";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / (projectId + ".drstrm")).generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    return project;
}

std::size_t countFindingsWithCode(const drs::engine::SfzImportAnalysisResult& analysis,
                                  const std::string& code)
{
    return static_cast<std::size_t>(
        std::count_if(analysis.report.findings.begin(),
                      analysis.report.findings.end(),
                      [&](const drs::engine::SfzImportFinding& finding)
                      {
                          return finding.code == code;
                      }));
}

struct FixtureExpectation
{
    const char* label = "";
    const char* relativePath = "";
    std::size_t expectedApproximateCount = 0;
    std::size_t expectedWarningCount = 0;
    std::string expectedSampleFragment;
};
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const std::vector<FixtureExpectation> fixtures {
            { "mono-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz",
              16,
              25,
              "jRhodes3d-mono" },
            { "mono-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono-no-xfade.sfz",
              0,
              9,
              "jRhodes3d-mono" },
            { "stereo-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st.sfz",
              16,
              25,
              "jRhodes3d-st" },
            { "stereo-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st-no-xfade.sfz",
              0,
              9,
              "jRhodes3d-st" },
            { "stereo-vibrato-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv.sfz",
              16,
              25,
              "jRhodes3d-sv" },
            { "stereo-vibrato-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv-no-xfade.sfz",
              0,
              9,
              "jRhodes3d-sv" }
        };

        for (const auto& fixture : fixtures)
        {
            const auto fixturePath = resolveFixturePath(fixture.relativePath);
            const auto analysis = analyzeSfzImportDocument(fixturePath.generic_string());

            require(analysis.analyzed,
                    std::string("Fixture should analyze successfully: ") + fixture.label);
            require(analysis.parseResult.parsed && analysis.parseResult.complete,
                    std::string("Fixture should parse completely: ") + fixture.label);
            require(analysis.normalizeResult.normalized,
                    std::string("Fixture should normalize completely: ") + fixture.label);
            require(analysis.report.available,
                    std::string("Fixture should publish a report: ") + fixture.label);
            require(!analysis.report.blocking,
                    std::string("Fixture should not be blocked: ") + fixture.label);
            require(analysis.report.stage == SfzImportStage::reviewReady,
                    std::string("Fixture should stay review-ready: ") + fixture.label);
            require(analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired,
                    std::string("Fixture should preserve the review confirmation gate: ") + fixture.label);

            require(analysis.report.summary.sourceFileCount == 1,
                    std::string("Fixture should still analyze as one source file: ") + fixture.label);
            require(analysis.report.summary.sectionCount == 233,
                    std::string("Fixture section count changed unexpectedly: ") + fixture.label);
            require(analysis.report.summary.opcodeCount
                        == analysis.report.summary.convertedOpcodeCount
                            + analysis.report.summary.approximatedOpcodeCount
                            + analysis.report.summary.reportedOnlyOpcodeCount
                            + analysis.report.summary.blockingOpcodeCount,
                    std::string("Fixture opcode summary should still balance exactly: ") + fixture.label);
            require(analysis.report.summary.convertedOpcodeCount == 1583,
                    std::string("Fixture converted-opcode count changed unexpectedly: ") + fixture.label);
            require(analysis.report.summary.approximatedOpcodeCount == fixture.expectedApproximateCount,
                    std::string("Fixture approximated-opcode count changed unexpectedly: ") + fixture.label);
            require(analysis.report.summary.reportedOnlyOpcodeCount == 9,
                    std::string("Fixture reported-only opcode count changed unexpectedly: ") + fixture.label);
            require(analysis.report.summary.blockingOpcodeCount == 0,
                    std::string("Fixture should not contribute blocking opcode counts: ") + fixture.label);
            require(analysis.report.summary.warningFindingCount == fixture.expectedWarningCount
                        && analysis.report.summary.errorFindingCount == 0,
                    std::string("Fixture warning/error counts changed unexpectedly: ") + fixture.label);
            require(analysis.report.findings.size() == fixture.expectedWarningCount,
                    std::string("Fixture finding count changed unexpectedly: ") + fixture.label);

            const auto crossfadeFindingCount =
                countFindingsWithCode(analysis, "sfz.velocity_crossfade.approximated");
            require(crossfadeFindingCount == fixture.expectedApproximateCount,
                    std::string("Fixture crossfade finding count changed unexpectedly: ") + fixture.label);

            const auto curveFindingCount = countFindingsWithCode(analysis, "sfz.curve.reported");
            require(curveFindingCount == 5,
                    std::string("Fixture should still surface every curve opcode: ") + fixture.label);
            require(countFindingsWithCode(analysis, "sfz.cc.label.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.default.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.width.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.width_curve.reported") == 1,
                    std::string("Fixture control-surface reporting changed unexpectedly: ") + fixture.label);

            const auto project = makeBlankPhase2Project(fixturePath, std::string("sprint31.") + fixture.label);
            const auto projection = projectSfzImportAnalysis(project, analysis);
            require(projection.projected,
                    std::string("Fixture should project into native content: ") + fixture.label);
            require(projection.playable,
                    std::string("Fixture projection should remain playable: ") + fixture.label);
            require(projection.lossy,
                    std::string("Fixture projection should preserve the review gate as lossy content: ") + fixture.label);
            require(!projection.blocking,
                    std::string("Fixture projection should not be blocked: ") + fixture.label);
            require(projection.sampleSources.size() == 195,
                    std::string("Fixture projected sample-source count changed unexpectedly: ") + fixture.label);
            require(projection.zones.size() == 225,
                    std::string("Fixture projected zone count changed unexpectedly: ") + fixture.label);
            require(!projection.projectNotes.empty() && !projection.authoringNotes.empty(),
                    std::string("Fixture projection should preserve import notes: ") + fixture.label);
            require(std::all_of(projection.sampleSources.begin(),
                                projection.sampleSources.end(),
                                [&](const RuntimeProjectSampleSource& sampleSource)
                                {
                                    return sampleSource.path.find(fixture.expectedSampleFragment) != std::string::npos;
                                }),
                    std::string("Fixture projected sample paths should stay rooted in the expected corpus folder: ")
                        + fixture.label);
        }

        std::cout << "Sprint 3.1.6 SFZ corpus hardening tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.6 SFZ corpus hardening tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
