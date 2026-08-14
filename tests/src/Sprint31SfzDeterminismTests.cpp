#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <fstream>
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
    project.displayName = "Sprint 3.1.6 Determinism";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / (projectId + ".drstrm")).generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    return project;
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

void requireFindingEquals(const drs::engine::SfzImportFinding& expected,
                          const drs::engine::SfzImportFinding& actual,
                          const std::string& context)
{
    require(expected.severity == actual.severity
                && expected.disposition == actual.disposition
                && expected.code == actual.code
                && expected.summary == actual.summary
                && expected.detail == actual.detail
                && expected.location.scope == actual.location.scope
                && expected.location.sourcePath == actual.location.sourcePath
                && expected.location.lineNumber == actual.location.lineNumber
                && expected.location.columnNumber == actual.location.columnNumber
                && expected.location.opcode == actual.location.opcode,
            context);
}

void requireTraceEquals(const drs::engine::SfzImportTraceEntry& expected,
                        const drs::engine::SfzImportTraceEntry& actual,
                        const std::string& context)
{
    require(expected.documentOrder == actual.documentOrder
                && expected.scope == actual.scope
                && expected.headerName == actual.headerName
                && expected.opcodeName == actual.opcodeName
                && expected.opcodeValue == actual.opcodeValue
                && expected.nativeTarget == actual.nativeTarget
                && expected.sampleReference == actual.sampleReference
                && expected.disposition == actual.disposition
                && expected.findingCode == actual.findingCode
                && expected.semanticDependencyKind == actual.semanticDependencyKind
                && expected.semanticImpact == actual.semanticImpact
                && expected.semanticSupport == actual.semanticSupport
                && expected.affectsRegionEligibility == actual.affectsRegionEligibility
                && expected.location.scope == actual.location.scope
                && expected.location.sourcePath == actual.location.sourcePath
                && expected.location.lineNumber == actual.location.lineNumber
                && expected.location.columnNumber == actual.location.columnNumber
                && expected.location.opcode == actual.location.opcode,
            context);
}

void requireSemanticDependencyEquals(const drs::engine::SfzImportSemanticDependency& expected,
                                     const drs::engine::SfzImportSemanticDependency& actual,
                                     const std::string& context)
{
    require(expected.kind == actual.kind
                && expected.impact == actual.impact
                && expected.support == actual.support
                && expected.affectsRegionEligibility == actual.affectsRegionEligibility
                && expected.controllerNumber == actual.controllerNumber
                && expected.opcodeName == actual.opcodeName
                && expected.opcodeValue == actual.opcodeValue
                && expected.inherited == actual.inherited
                && expected.location.scope == actual.location.scope
                && expected.location.sourcePath == actual.location.sourcePath
                && expected.location.lineNumber == actual.location.lineNumber
                && expected.location.columnNumber == actual.location.columnNumber
                && expected.location.opcode == actual.location.opcode,
            context);
}

void requireRegionSemanticAnalysisEquals(const drs::engine::SfzImportRegionSemanticAnalysis& expected,
                                         const drs::engine::SfzImportRegionSemanticAnalysis& actual,
                                         const std::string& context)
{
    require(expected.documentOrder == actual.documentOrder
                && expected.sampleReference == actual.sampleReference
                && expected.hasSoundCriticalDependencies == actual.hasSoundCriticalDependencies
                && expected.hasIncompleteSoundCriticalDependencies
                    == actual.hasIncompleteSoundCriticalDependencies
                && expected.safeToProjectUnconditionally == actual.safeToProjectUnconditionally
                && expected.dependencies.size() == actual.dependencies.size(),
            context);

    for (std::size_t index = 0; index < expected.dependencies.size(); ++index)
        requireSemanticDependencyEquals(expected.dependencies[index],
                                        actual.dependencies[index],
                                        context + " dependency mismatch at index " + std::to_string(index));
}

void requireOmittedRegionSummaryEquals(const drs::engine::SfzImportOmittedRegionSummary& expected,
                                       const drs::engine::SfzImportOmittedRegionSummary& actual,
                                       const std::string& context)
{
    require(expected.dependencyKind == actual.dependencyKind
                && expected.controllerNumber == actual.controllerNumber
                && expected.sourceScope == actual.sourceScope
                && expected.sourcePath == actual.sourcePath
                && expected.firstSourceLineNumber == actual.firstSourceLineNumber
                && expected.feature == actual.feature
                && expected.affectedRegionCount == actual.affectedRegionCount,
            context);
}

void requireSupportSummaryEquals(const drs::engine::SfzImportOpcodeSupportSummary& expected,
                                 const drs::engine::SfzImportOpcodeSupportSummary& actual,
                                 const std::string& context)
{
    require(expected.scope == actual.scope
                && expected.opcodeName == actual.opcodeName
                && expected.disposition == actual.disposition
                && expected.nativeTarget == actual.nativeTarget
                && expected.rationale == actual.rationale
                && expected.occurrenceCount == actual.occurrenceCount,
            context);
}

void requireSampleSourceEquals(const drs::engine::RuntimeProjectSampleSource& expected,
                               const drs::engine::RuntimeProjectSampleSource& actual,
                               const std::string& context)
{
    require(expected.id == actual.id
                && expected.path == actual.path
                && expected.role == actual.role,
            context);
}

void requireGroupEquals(const drs::engine::RuntimeProjectGroupDefinition& expected,
                        const drs::engine::RuntimeProjectGroupDefinition& actual,
                        const std::string& context)
{
    require(expected.id == actual.id
                && expected.displayName == actual.displayName
                && expected.displayOrder == actual.displayOrder
                && expected.workspaceVisible == actual.workspaceVisible
                && expected.gainDb == actual.gainDb
                && expected.pan == actual.pan
                && expected.routingBusId == actual.routingBusId
                && expected.auditionAnchorZoneId == actual.auditionAnchorZoneId,
            context);
}

void requireZoneEquals(const drs::engine::RuntimeProjectZoneDefinition& expected,
                       const drs::engine::RuntimeProjectZoneDefinition& actual,
                       const std::string& context)
{
    require(expected.id == actual.id
                && expected.sampleSourceId == actual.sampleSourceId
                && expected.displayName == actual.displayName
                && expected.groupId == actual.groupId
                && expected.articulationId == actual.articulationId
                && expected.rootKey == actual.rootKey
                && expected.keyLow == actual.keyLow
                && expected.keyHigh == actual.keyHigh
                && expected.velocityLow == actual.velocityLow
                && expected.velocityHigh == actual.velocityHigh
                && expected.velocityCrossfade.fadeInLowVelocity == actual.velocityCrossfade.fadeInLowVelocity
                && expected.velocityCrossfade.fadeInHighVelocity == actual.velocityCrossfade.fadeInHighVelocity
                && expected.velocityCrossfade.fadeOutLowVelocity == actual.velocityCrossfade.fadeOutLowVelocity
                && expected.velocityCrossfade.fadeOutHighVelocity == actual.velocityCrossfade.fadeOutHighVelocity
                && expected.gainDb == actual.gainDb
                && expected.pan == actual.pan
                && expected.sampleStartFrame == actual.sampleStartFrame
                && expected.loopEnabled == actual.loopEnabled
                && expected.loopStartFrame == actual.loopStartFrame
                && expected.loopEndFrame == actual.loopEndFrame
                && expected.releaseSeconds == actual.releaseSeconds
                && expected.releaseShape == actual.releaseShape
                && expected.roundRobinLength == actual.roundRobinLength
                && expected.roundRobinPosition == actual.roundRobinPosition
                && expected.triggerMode == actual.triggerMode,
            context);
}

void requireProjectionEquals(const drs::engine::SfzImportProjectionResult& expected,
                             const drs::engine::SfzImportProjectionResult& actual,
                             const std::string& context)
{
    require(expected.projected == actual.projected
                && expected.playable == actual.playable
                && expected.lossy == actual.lossy
                && expected.blocking == actual.blocking
                && expected.state == actual.state
                && expected.issues == actual.issues
                && expected.semanticAnalyzedRegionCount == actual.semanticAnalyzedRegionCount
                && expected.unsafeUnconditionalRegionCount == actual.unsafeUnconditionalRegionCount
                && expected.unsafeUnconditionalRegionDocumentOrders
                    == actual.unsafeUnconditionalRegionDocumentOrders
                && expected.omittedUnsafeRegionCount == actual.omittedUnsafeRegionCount
                && expected.masterGainDb == actual.masterGainDb
                && expected.projectNotes == actual.projectNotes
                && expected.authoringNotes == actual.authoringNotes
                && expected.sampleSources.size() == actual.sampleSources.size()
                && expected.groups.size() == actual.groups.size()
                && expected.zones.size() == actual.zones.size()
                && expected.omittedRegionSummaries.size()
                    == actual.omittedRegionSummaries.size(),
            context);

    for (std::size_t index = 0; index < expected.omittedRegionSummaries.size(); ++index)
        requireOmittedRegionSummaryEquals(
            expected.omittedRegionSummaries[index],
            actual.omittedRegionSummaries[index],
            context + " omission summary mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.sampleSources.size(); ++index)
        requireSampleSourceEquals(expected.sampleSources[index],
                                  actual.sampleSources[index],
                                  context + " sample source mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.groups.size(); ++index)
        requireGroupEquals(expected.groups[index],
                           actual.groups[index],
                           context + " group mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.zones.size(); ++index)
        requireZoneEquals(expected.zones[index],
                          actual.zones[index],
                          context + " zone mismatch at index " + std::to_string(index));
}

void requireAnalysisEquals(const drs::engine::SfzImportAnalysisResult& expected,
                           const drs::engine::SfzImportAnalysisResult& actual,
                           const std::string& context)
{
    require(expected.analyzed == actual.analyzed
                && expected.parseResult.parsed == actual.parseResult.parsed
                && expected.parseResult.complete == actual.parseResult.complete
                && expected.parseResult.state == actual.parseResult.state
                && expected.normalizeResult.normalized == actual.normalizeResult.normalized
                && expected.normalizeResult.state == actual.normalizeResult.state
                && expected.report.available == actual.report.available
                && expected.report.blocking == actual.report.blocking
                && expected.report.stage == actual.report.stage
                && expected.report.reviewDisposition == actual.report.reviewDisposition
                && expected.report.state == actual.report.state
                && expected.report.rootDocumentPath == actual.report.rootDocumentPath
                && expected.report.sourceFiles == actual.report.sourceFiles,
            context);

    const auto& expectedSummary = expected.report.summary;
    const auto& actualSummary = actual.report.summary;
    require(expectedSummary.sourceFileCount == actualSummary.sourceFileCount
                && expectedSummary.sectionCount == actualSummary.sectionCount
                && expectedSummary.opcodeCount == actualSummary.opcodeCount
                && expectedSummary.convertedOpcodeCount == actualSummary.convertedOpcodeCount
                && expectedSummary.approximatedOpcodeCount == actualSummary.approximatedOpcodeCount
                && expectedSummary.reportedOnlyOpcodeCount == actualSummary.reportedOnlyOpcodeCount
                && expectedSummary.blockingOpcodeCount == actualSummary.blockingOpcodeCount
                && expectedSummary.informationFindingCount == actualSummary.informationFindingCount
                && expectedSummary.warningFindingCount == actualSummary.warningFindingCount
                && expectedSummary.errorFindingCount == actualSummary.errorFindingCount
                && expectedSummary.semanticAnalyzedRegionCount
                    == actualSummary.semanticAnalyzedRegionCount
                && expectedSummary.semanticDependencyCount == actualSummary.semanticDependencyCount
                && expectedSummary.soundCriticalDependencyCount
                    == actualSummary.soundCriticalDependencyCount
                && expectedSummary.incompleteSoundCriticalDependencyCount
                    == actualSummary.incompleteSoundCriticalDependencyCount
                && expectedSummary.presentationOnlyDependencyCount
                    == actualSummary.presentationOnlyDependencyCount
                && expectedSummary.unsafeUnconditionalRegionCount
                    == actualSummary.unsafeUnconditionalRegionCount,
            context + " summary mismatch");

    require(expected.report.findings.size() == actual.report.findings.size()
                && expected.report.traceEntries.size() == actual.report.traceEntries.size()
                && expected.report.opcodeSupport.size() == actual.report.opcodeSupport.size()
                && expected.report.regionSemanticAnalysis.size()
                    == actual.report.regionSemanticAnalysis.size(),
            context + " vector size mismatch");

    for (std::size_t index = 0; index < expected.report.findings.size(); ++index)
        requireFindingEquals(expected.report.findings[index],
                             actual.report.findings[index],
                             context + " finding mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.report.traceEntries.size(); ++index)
        requireTraceEquals(expected.report.traceEntries[index],
                           actual.report.traceEntries[index],
                           context + " trace mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.report.opcodeSupport.size(); ++index)
        requireSupportSummaryEquals(expected.report.opcodeSupport[index],
                                    actual.report.opcodeSupport[index],
                                    context + " support-summary mismatch at index " + std::to_string(index));

    for (std::size_t index = 0; index < expected.report.regionSemanticAnalysis.size(); ++index)
        requireRegionSemanticAnalysisEquals(expected.report.regionSemanticAnalysis[index],
                                            actual.report.regionSemanticAnalysis[index],
                                            context + " region semantic mismatch at index "
                                                + std::to_string(index));
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const std::vector<fs::path> fixtures {
            fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz"),
            fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st-no-xfade.sfz")
        };

        for (const auto& fixture : fixtures)
        {
            const auto fixturePath = resolveFixturePath(fixture);
            const auto analysisFirst = analyzeSfzImportDocument(fixturePath.generic_string());
            const auto analysisSecond = analyzeSfzImportDocument(fixturePath.generic_string());
            requireAnalysisEquals(analysisFirst,
                                  analysisSecond,
                                  "Repeated analysis must remain deterministic for " + fixture.generic_string());

            const auto baseProject =
                makeBlankPhase2Project(fixturePath, "sprint31.determinism." + fixture.stem().generic_string());
            const auto projectionFromAnalysis = projectSfzImportAnalysis(baseProject, analysisFirst);
            const auto projectionFromDocument = projectSfzImportDocument(baseProject, fixturePath.generic_string());
            const auto projectionRepeated = projectSfzImportAnalysis(baseProject, analysisSecond);

            requireProjectionEquals(projectionFromAnalysis,
                                    projectionFromDocument,
                                    "Projection from analysis and direct document import must match for "
                                        + fixture.generic_string());
            requireProjectionEquals(projectionFromAnalysis,
                                    projectionRepeated,
                                    "Repeated projection must remain deterministic for " + fixture.generic_string());
        }

        const auto stereoNoXfadePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st-no-xfade.sfz");
        const auto stereoProject = makeBlankPhase2Project(stereoNoXfadePath, "sprint31.determinism.stereo-no-xfade");
        const auto stereoProjection = projectSfzImportDocument(stereoProject, stereoNoXfadePath.generic_string());
        require(stereoProjection.projected && stereoProjection.playable,
                "The broadened stereo no-xfade corpus fixture should still project into playable native content.");

        AuthoringSession session(stereoProject);
        const auto applyResult = applySfzImportProjection(session,
                                                          stereoProjection,
                                                          "Import Sprint 3.1.6 stereo no-xfade SFZ fixture");
        require(applyResult.applied,
                "The broadened stereo no-xfade corpus fixture should still apply through the authoring session.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-sprint31-sfz-determinism-tests";
        const auto projectPath = tempDirectory / "sfz-stereo-no-xfade-roundtrip.drsproj";
        auto streamPath = projectPath;
        streamPath.replace_extension(".drstrm");
        auto instrumentPath = projectPath;
        instrumentPath.replace_extension(".drinst");

        auto savedProject = session.getProject();
        savedProject.defaultInstrumentManifestPath = instrumentPath.generic_string();
        const auto instrument = drs::app::buildInstrumentManifestForProject(savedProject,
                                                                            juce::File(projectPath.generic_string()));
        require(instrument.zones.size() == savedProject.authoring.zones.size(),
                "Broadened save/load hardening should preserve every projected stereo no-xfade zone.");

        writeTextFile(streamPath, "determinism stream placeholder");
        writeTextFile(instrumentPath,
                      serializeRuntimeInstrumentManifest(instrument, instrumentPath.generic_string()));
        writeTextFile(projectPath,
                      serializeRuntimeProjectManifest(savedProject, projectPath.generic_string()));

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded,
                "The broadened stereo no-xfade corpus fixture should survive project save/load round-tripping.");
        require(roundTripProject.project.sampleSources.size() == savedProject.sampleSources.size()
                    && roundTripProject.project.authoring.zones.size() == savedProject.authoring.zones.size(),
                "Broadened project round-tripping should preserve stereo no-xfade sample-source and zone counts.");
        require(roundTripProject.project.authoring.zones.front().roundRobinLength == 3
                    && roundTripProject.project.authoring.zones.front().roundRobinPosition == 1
                    && roundTripProject.project.authoring.zones.front().releaseSeconds == 0.5
                    && roundTripProject.project.authoring.zones.front().releaseShape == sfzDefaultReleaseShape,
                "Broadened project round-tripping should preserve stereo no-xfade round-robin and release metadata.");

        const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
        require(roundTripInstrument.loaded,
                "The broadened stereo no-xfade corpus fixture should survive instrument save/load round-tripping.");
        require(roundTripInstrument.instrument.zones.size() == instrument.zones.size()
                    && roundTripInstrument.instrument.zones.front().roundRobinLength == 3
                    && roundTripInstrument.instrument.zones.front().roundRobinPosition == 1
                    && roundTripInstrument.instrument.zones.front().releaseSeconds == 0.5
                    && roundTripInstrument.instrument.zones.front().releaseShape == sfzDefaultReleaseShape,
                "Broadened instrument round-tripping should preserve stereo no-xfade round-robin and release metadata.");

        std::cout << "Sprint 3.1.6 SFZ determinism tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.6 SFZ determinism tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
