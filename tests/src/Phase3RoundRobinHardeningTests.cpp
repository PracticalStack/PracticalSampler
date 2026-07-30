#include "drs/engine/AuthoringSession.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>

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
    project.displayName = "Phase 3 Round Robin Sprint 8 Hardening";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath =
        (fixturePath.parent_path() / (projectId + ".drinst")).generic_string();
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

std::size_t countZonesWithExplicitRoundRobin(
    const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones)
{
    return static_cast<std::size_t>(
        std::count_if(zones.begin(),
                      zones.end(),
                      [](const drs::engine::RuntimeProjectZoneDefinition& zone)
                      {
                          return zone.roundRobin.has_value();
                      }));
}

std::size_t countZonesWithCrossfade(
    const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones)
{
    return static_cast<std::size_t>(
        std::count_if(zones.begin(),
                      zones.end(),
                      [](const drs::engine::RuntimeProjectZoneDefinition& zone)
                      {
                          return drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade);
                      }));
}

bool hasMixedRoundRobinCrossfadeSlot(
    const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones)
{
    struct SlotSummary
    {
        std::size_t zoneCount = 0;
        bool hasFadeIn = false;
        bool hasFadeOut = false;
    };

    std::map<std::string, SlotSummary> summaries;
    for (const auto& zone : zones)
    {
        if (!zone.roundRobin.has_value())
            continue;

        auto& summary = summaries[zone.roundRobin->poolId + "#" + std::to_string(zone.roundRobin->slotIndex)];
        ++summary.zoneCount;
        summary.hasFadeIn = summary.hasFadeIn
            || zone.velocityCrossfade.fadeInLowVelocity > 0
            || zone.velocityCrossfade.fadeInHighVelocity > 0;
        summary.hasFadeOut = summary.hasFadeOut
            || zone.velocityCrossfade.fadeOutLowVelocity > 0
            || zone.velocityCrossfade.fadeOutHighVelocity > 0;
    }

    return std::any_of(summaries.begin(),
                       summaries.end(),
                       [](const auto& entry)
                       {
                           const auto& summary = entry.second;
                           return summary.zoneCount >= 2 && summary.hasFadeIn && summary.hasFadeOut;
                       });
}

void requireImportedProjectShape(const drs::engine::RuntimeProjectModel& project,
                                 const std::string& context)
{
    require(project.schemaVersion == 3 && project.authoring.schemaVersion == 2,
            context + " should migrate into the Phase 3 Round Robin schema.");
    require(project.sampleSources.size() == 195,
            context + " should preserve all imported sample sources.");
    require(project.authoring.zones.size() == 225,
            context + " should preserve all imported zones.");
    require(countZonesWithExplicitRoundRobin(project.authoring.zones) == 225,
            context + " should carry explicit Round Robin descriptors on every imported zone.");
    require(countZonesWithCrossfade(project.authoring.zones) > 0,
            context + " should preserve the mixed RR-plus-crossfade subset.");
    require(hasMixedRoundRobinCrossfadeSlot(project.authoring.zones),
            context + " should preserve at least one same-slot RR crossfade pair.");
}

struct PublishGateSnapshot
{
    drs::engine::EnginePerformanceSnapshot performance;
    drs::engine::EngineDiagnosticsSnapshot diagnostics;
};

PublishGateSnapshot previewAndPublish(
    const drs::engine::RuntimeProjectModel& project,
    std::size_t revision)
{
    drs::engine::EngineFacade engineFacade;
    require(engineFacade.replaceDraftPlaybackAuthoringProject(project),
            "Engine facade should accept the imported Round Robin hardening project.");
    require(engineFacade.stageDraftRevision(revision),
            "Engine facade should stage the imported Round Robin hardening revision.");
    require(engineFacade.refreshPreviewToCurrentDraft(),
            "Engine facade should prepare Preview for the imported Round Robin hardening project.");
    require(engineFacade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(10000)),
            "Round Robin hardening Preview should settle through the prepared worker.");
    engineFacade.serviceBackgroundWork();
    require(engineFacade.publishCurrentDraft(),
            "Engine facade should publish the imported Round Robin hardening draft.");
    require(engineFacade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(10000)),
            "Round Robin hardening Publish should settle through the prepared worker.");
    engineFacade.serviceBackgroundWork();

    const auto performanceSnapshot = engineFacade.getPerformanceSnapshot();
    const auto diagnosticsSnapshot = engineFacade.getDiagnosticsSnapshot();
    require(performanceSnapshot.loaded,
            "Round Robin hardening performance snapshot should remain loaded after publish.");
    require(performanceSnapshot.previewRevision == revision
                && performanceSnapshot.publishedRevision == revision,
            "Round Robin hardening Preview and Publish should converge on the imported revision.");
    require(performanceSnapshot.previewRevisionState == "Ready"
                && performanceSnapshot.publishedPresentationState
                    == drs::engine::PerformancePublishPresentationState::ready,
            "Round Robin hardening Preview and Publish should both become ready.");
    require(performanceSnapshot.previewActivationEligible
                && performanceSnapshot.publishedActivationEligible,
            "Round Robin hardening Preview and Publish should remain activation-eligible.");
    require(diagnosticsSnapshot.previewPreparedZoneCount == 225
                && diagnosticsSnapshot.publishedPreparedZoneCount == 225,
            "Round Robin hardening Preview and Publish should preserve the full prepared route set.");
    require(performanceSnapshot.previewPreparedContentDigest
                == performanceSnapshot.publishedPreparedContentDigest,
            "Round Robin hardening Preview and Publish should expose matching prepared digests.");
    require(performanceSnapshot.previewContentDigest == performanceSnapshot.publishedContentDigest,
            "Round Robin hardening Preview and Publish should expose matching snapshot digests.");
    require(!performanceSnapshot.publishedRouteDigest.empty(),
            "Round Robin hardening Publish should expose a non-empty route digest.");
    require(performanceSnapshot.previewFindings.empty()
                && performanceSnapshot.publishedFindings.empty(),
            "Round Robin hardening Preview and Publish should not introduce runtime findings.");
    return { performanceSnapshot, diagnosticsSnapshot };
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixturePath = resolveFixturePath(
            "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");
        const auto analysisFirst = analyzeSfzImportDocument(fixturePath.generic_string());
        const auto analysisSecond = analyzeSfzImportDocument(fixturePath.generic_string());

        require(analysisFirst.analyzed && analysisSecond.analyzed,
                "The mixed RR-plus-crossfade hardening fixture should analyze successfully.");
        require(analysisFirst.report.available && analysisSecond.report.available,
                "The mixed RR-plus-crossfade hardening fixture should publish import reports.");
        require(analysisFirst.report.summary.convertedOpcodeCount
                        == analysisSecond.report.summary.convertedOpcodeCount
                    && analysisFirst.report.summary.reportedOnlyOpcodeCount
                        == analysisSecond.report.summary.reportedOnlyOpcodeCount
                    && analysisFirst.report.findings.size() == analysisSecond.report.findings.size(),
                "Repeated RR hardening analysis should remain deterministic.");

        const auto baseProject = makeBlankPhase2Project(fixturePath, "phase3.round-robin.sprint8");
        const auto projectionFirst = projectSfzImportAnalysis(baseProject, analysisFirst);
        const auto projectionSecond = projectSfzImportAnalysis(baseProject, analysisSecond);

        require(projectionFirst.projected && projectionFirst.playable && !projectionFirst.blocking,
                "The mixed RR-plus-crossfade hardening fixture should project into playable native content.");
        require(projectionSecond.projected && projectionSecond.playable && !projectionSecond.blocking,
                "Repeated RR hardening projection should remain playable.");
        require(projectionFirst.sampleSources.size() == projectionSecond.sampleSources.size()
                    && projectionFirst.zones.size() == projectionSecond.zones.size(),
                "Repeated RR hardening projection should preserve source and zone counts.");

        AuthoringSession firstSession(baseProject);
        const auto firstApply = applySfzImportProjection(firstSession,
                                                         projectionFirst,
                                                         "Apply Phase 3 Round Robin Sprint 8 hardening fixture");
        require(firstApply.applied,
                "The mixed RR-plus-crossfade hardening fixture should apply through the authoring session."
                    + (firstApply.issues.empty() ? std::string()
                                                : " First issue: " + firstApply.issues.front()));

        AuthoringSession secondSession(baseProject);
        const auto secondApply = applySfzImportProjection(secondSession,
                                                          projectionSecond,
                                                          "Apply Phase 3 Round Robin Sprint 8 hardening fixture");
        require(secondApply.applied,
                "Repeated RR hardening projection should apply through the authoring session.");

        auto firstProject = firstSession.getProject();
        auto secondProject = secondSession.getProject();
        requireImportedProjectShape(firstProject, "First imported hardening project");
        requireImportedProjectShape(secondProject, "Second imported hardening project");
        require(firstApply.documentState.revision == secondApply.documentState.revision,
                "Repeated RR hardening imports should land on the same authoring revision.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase3-round-robin-hardening-tests";
        const auto projectPath = tempDirectory / "round-robin-hardening.drsproj";
        const auto instrumentPath = tempDirectory / "round-robin-hardening.drinst";
        const auto streamPath = tempDirectory / "round-robin-hardening.drstrm";

        firstProject.defaultInstrumentManifestPath = instrumentPath.generic_string();
        secondProject.defaultInstrumentManifestPath = instrumentPath.generic_string();

        const auto serializedProjectFirst =
            serializeRuntimeProjectManifest(firstProject, projectPath.generic_string());
        const auto serializedProjectSecond =
            serializeRuntimeProjectManifest(secondProject, projectPath.generic_string());
        require(serializedProjectFirst == serializedProjectSecond,
                "Repeated RR hardening import should serialize identical native project manifests.");
        writeTextFile(instrumentPath, "phase3 round robin hardening instrument placeholder");
        writeTextFile(projectPath, serializedProjectFirst);

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded,
                "The mixed RR-plus-crossfade hardening project should survive project save/load round-tripping.");
        requireImportedProjectShape(roundTripProject.project, "Round-tripped hardening project");
        require(serializeRuntimeProjectManifest(roundTripProject.project, projectPath.generic_string())
                    == serializedProjectFirst,
                "Round-tripped RR hardening project should reserialize deterministically.");

        const auto instrumentFirst =
            drs::app::buildInstrumentManifestForProject(firstProject, juce::File(projectPath.generic_string()));
        const auto instrumentSecond =
            drs::app::buildInstrumentManifestForProject(secondProject, juce::File(projectPath.generic_string()));
        require(instrumentFirst.zones.size() == 225 && instrumentSecond.zones.size() == 225,
                "RR hardening project-to-instrument conversion should preserve the full zone set.");
        writeTextFile(streamPath, "phase3 round robin hardening stream placeholder");

        const auto serializedInstrumentFirst =
            serializeRuntimeInstrumentManifest(instrumentFirst, instrumentPath.generic_string());
        const auto serializedInstrumentSecond =
            serializeRuntimeInstrumentManifest(instrumentSecond, instrumentPath.generic_string());
        require(serializedInstrumentFirst == serializedInstrumentSecond,
                "Repeated RR hardening import should serialize identical instrument manifests.");
        writeTextFile(instrumentPath, serializedInstrumentFirst);

        const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
        require(roundTripInstrument.loaded,
                "The mixed RR-plus-crossfade hardening instrument should survive instrument save/load round-tripping.");
        require(roundTripInstrument.instrument.zones.size() == 225,
                "Round-tripped RR hardening instrument should preserve the full zone set.");
        require(static_cast<std::size_t>(
                    std::count_if(roundTripInstrument.instrument.zones.begin(),
                                  roundTripInstrument.instrument.zones.end(),
                                  [](const RuntimeZoneDefinition& zone)
                                  {
                                      return zone.roundRobin.has_value();
                                  }))
                    == 225,
                "Round-tripped RR hardening instrument should preserve explicit Round Robin descriptors.");

        const auto performanceFirst = previewAndPublish(firstProject, firstApply.documentState.revision);
        const auto performanceSecond = previewAndPublish(secondProject, secondApply.documentState.revision);
        require(performanceFirst.performance.previewPreparedContentDigest
                        == performanceSecond.performance.previewPreparedContentDigest
                    && performanceFirst.performance.publishedPreparedContentDigest
                        == performanceSecond.performance.publishedPreparedContentDigest
                    && performanceFirst.performance.publishedRouteDigest
                        == performanceSecond.performance.publishedRouteDigest,
                "Repeated RR hardening preview/publish runs should remain deterministic.");

        std::cout << "Phase 3 Round Robin Sprint 8 hardening tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 Round Robin Sprint 8 hardening tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
