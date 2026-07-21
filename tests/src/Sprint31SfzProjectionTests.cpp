#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path resolveFirstFixturePath()
{
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);
    const auto relativeFixturePath =
        fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (fs::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (fs::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

drs::engine::RuntimeProjectModel makeBlankPhase2Project(const fs::path& fixturePath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = "sprint31.sfz-projection";
    project.displayName = "Sprint 3.1.4 Projection";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / "projection-test.drstrm").generic_string();
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

void requireCrossfadeEquals(const drs::engine::VelocityCrossfadeDescriptor& crossfade,
                            int fadeInLowVelocity,
                            int fadeInHighVelocity,
                            int fadeOutLowVelocity,
                            int fadeOutHighVelocity,
                            const std::string& context)
{
    require(crossfade.fadeInLowVelocity == fadeInLowVelocity,
            context + " should preserve fadeInLowVelocity.");
    require(crossfade.fadeInHighVelocity == fadeInHighVelocity,
            context + " should preserve fadeInHighVelocity.");
    require(crossfade.fadeOutLowVelocity == fadeOutLowVelocity,
            context + " should preserve fadeOutLowVelocity.");
    require(crossfade.fadeOutHighVelocity == fadeOutHighVelocity,
            context + " should preserve fadeOutHighVelocity.");
}

void requireCrossfadeRuntimeEquals(const drs::engine::VelocityCrossfadeRuntimeDescriptor& runtime,
                                   int effectiveLowVelocity,
                                   int effectiveHighVelocity,
                                   const std::string& fadeInNeighborZoneId,
                                   const std::string& fadeOutNeighborZoneId,
                                   int fadeInOverlapLowVelocity,
                                   int fadeInOverlapHighVelocity,
                                   int fadeOutOverlapLowVelocity,
                                   int fadeOutOverlapHighVelocity,
                                   const std::string& context)
{
    require(runtime.effectiveLowVelocity == effectiveLowVelocity,
            context + " should preserve effectiveLowVelocity.");
    require(runtime.effectiveHighVelocity == effectiveHighVelocity,
            context + " should preserve effectiveHighVelocity.");
    require(runtime.fadeInNeighborZoneId == fadeInNeighborZoneId,
            context + " should preserve fadeInNeighborZoneId.");
    require(runtime.fadeOutNeighborZoneId == fadeOutNeighborZoneId,
            context + " should preserve fadeOutNeighborZoneId.");
    require(runtime.fadeInOverlapLowVelocity == fadeInOverlapLowVelocity,
            context + " should preserve fadeInOverlapLowVelocity.");
    require(runtime.fadeInOverlapHighVelocity == fadeInOverlapHighVelocity,
            context + " should preserve fadeInOverlapHighVelocity.");
    require(runtime.fadeOutOverlapLowVelocity == fadeOutOverlapLowVelocity,
            context + " should preserve fadeOutOverlapLowVelocity.");
    require(runtime.fadeOutOverlapHighVelocity == fadeOutOverlapHighVelocity,
            context + " should preserve fadeOutOverlapHighVelocity.");
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto fixturePath = resolveFirstFixturePath();
        const auto blankProject = makeBlankPhase2Project(fixturePath);
        const auto analysis = analyzeSfzImportDocument(fixturePath.generic_string());
        const auto projection = projectSfzImportAnalysis(blankProject, analysis);

        require(projection.projected, "Sprint 3.1.4 should project the first SFZ fixture into native content.");
        require(projection.playable, "Sprint 3.1.4 should keep the first SFZ fixture playable after projection.");
        require(projection.lossy, "The first SFZ fixture should still require lossy-review notes in Sprint 3.1.4.");
        require(!projection.blocking, "The first SFZ fixture should not block projection when every sample resolves.");
        require(projection.sampleSources.size() == 195,
                "The first SFZ fixture should still project its 195 unique sample sources.");
        require(projection.zones.size() == 225,
                "The first SFZ fixture should still create one projected zone per region.");
        require(!projection.projectNotes.empty(),
                "Sprint 3.1.4 should persist at least one project-level SFZ provenance note.");
        require(!projection.authoringNotes.empty(),
                "Sprint 3.1.4 should persist creator-facing SFZ compatibility notes.");

        const auto& firstZone = projection.zones.at(0);
        const auto& secondZone = projection.zones.at(1);
        const auto& thirdZone = projection.zones.at(2);
        const auto& secondLayerFirstZone = projection.zones.at(45);
        require(firstZone.roundRobinLength == 3
                    && secondZone.roundRobinLength == 3
                    && thirdZone.roundRobinLength == 3,
                "The first projected SFZ regions should preserve 3-way round-robin depth.");
        require(firstZone.roundRobinPosition == 1
                    && secondZone.roundRobinPosition == 2
                    && thirdZone.roundRobinPosition == 3,
                "The first projected SFZ regions should preserve round-robin positions 1, 2, and 3.");
        require(firstZone.gainDb == 6.0,
                "The projected SFZ master volume should map into native zone gain.");
        require(firstZone.releaseSeconds == 0.5,
                "The projected SFZ ampeg_release should land on the first region.");
        require(firstZone.sampleSourceId != secondZone.sampleSourceId
                    && secondZone.sampleSourceId != thirdZone.sampleSourceId,
                "The first round-robin regions should still point at distinct sample sources.");
        requireCrossfadeEquals(firstZone.velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "The first projected SFZ region crossfade metadata");
        requireCrossfadeEquals(secondLayerFirstZone.velocityCrossfade,
                               25,
                               60,
                               61,
                               84,
                               "The second projected SFZ layer crossfade metadata");

        AuthoringSession session(blankProject);
        const auto applyResult = applySfzImportProjection(session, projection, "Import Sprint 3.1.4 SFZ fixture");
        require(applyResult.applied, "Sprint 3.1.4 should apply the projected SFZ content through an undo-safe authoring commit.");
        require(session.getProject().sampleSources.size() == 195,
                "Applying the first SFZ fixture should append the projected sample sources.");
        require(session.getProject().authoring.zones.size() == 225,
                "Applying the first SFZ fixture should append the projected zones.");
        require(!session.getProject().notes.empty() && !session.getProject().authoring.notes.empty(),
                "Applying the first SFZ fixture should persist both project and authoring SFZ notes.");
        require(session.getDocumentState().revision == 1 && session.getDocumentState().undoDepth == 1,
                "Applying the first SFZ fixture should create exactly one undoable authoring revision.");

        const auto undoResult = session.undo();
        require(undoResult.applied, "The projected SFZ import should be undoable.");
        require(session.getProject().sampleSources.empty()
                    && session.getProject().authoring.zones.empty()
                    && session.getProject().notes.empty()
                    && session.getProject().authoring.notes.empty(),
                "Undo should fully remove the projected SFZ content and saved notes.");

        const auto redoResult = session.redo();
        require(redoResult.applied, "The projected SFZ import should be redoable.");
        require(session.getProject().sampleSources.size() == 195
                    && session.getProject().authoring.zones.size() == 225,
                "Redo should fully restore the projected SFZ content.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-sprint31-sfz-projection-tests";
        const auto projectPath = tempDirectory / "sfz-projection-roundtrip.drsproj";
        auto streamPath = projectPath;
        streamPath.replace_extension(".drstrm");
        auto instrumentPath = projectPath;
        instrumentPath.replace_extension(".drinst");
        auto savedProject = session.getProject();
        savedProject.defaultInstrumentManifestPath = instrumentPath.generic_string();
        const auto instrument = drs::app::buildInstrumentManifestForProject(
            savedProject,
            juce::File(projectPath.generic_string()));
        require(instrument.zones.size() == 225,
                "Projected SFZ content should build a native instrument manifest with every zone.");
        require(instrument.zones.at(0).roundRobinLength == 3
                    && instrument.zones.at(1).roundRobinPosition == 2
                    && instrument.zones.at(0).releaseSeconds == 0.5,
                "Projected SFZ round-robin and release metadata should survive project-to-instrument conversion.");
        requireCrossfadeEquals(instrument.zones.at(0).velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Projected first instrument zone crossfade metadata");
        requireCrossfadeEquals(instrument.zones.at(45).velocityCrossfade,
                               25,
                               60,
                               61,
                               84,
                               "Projected second instrument layer crossfade metadata");
        requireCrossfadeRuntimeEquals(instrument.zones.at(0).velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      instrument.zones.at(45).id,
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Projected first instrument zone runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(instrument.zones.at(45).velocityCrossfadeRuntime,
                                      25,
                                      84,
                                      instrument.zones.at(0).id,
                                      instrument.zones.at(90).id,
                                      25,
                                      60,
                                      61,
                                      84,
                                      "Projected second instrument layer runtime crossfade metadata");

        writeTextFile(streamPath, "projection stream placeholder");
        writeTextFile(instrumentPath,
                      serializeRuntimeInstrumentManifest(instrument, instrumentPath.generic_string()));
        writeTextFile(projectPath,
                      serializeRuntimeProjectManifest(savedProject, projectPath.generic_string()));

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded, "Projected SFZ content should survive project save/load round-tripping.");
        require(roundTripProject.project.authoring.zones.at(0).roundRobinLength == 3
                    && roundTripProject.project.authoring.zones.at(0).roundRobinPosition == 1
                    && roundTripProject.project.authoring.zones.at(0).releaseSeconds == 0.5,
                "Projected SFZ zone metadata should survive project round-tripping.");
        requireCrossfadeEquals(roundTripProject.project.authoring.zones.at(0).velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Round-tripped projected first project zone crossfade metadata");
        requireCrossfadeEquals(roundTripProject.project.authoring.zones.at(45).velocityCrossfade,
                               25,
                               60,
                               61,
                               84,
                               "Round-tripped projected second project layer crossfade metadata");

        const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
        require(roundTripInstrument.loaded,
                "Projected SFZ content should survive instrument-manifest save/load round-tripping.");
        require(roundTripInstrument.instrument.zones.at(0).roundRobinLength == 3
                    && roundTripInstrument.instrument.zones.at(2).roundRobinPosition == 3
                    && roundTripInstrument.instrument.zones.at(0).releaseSeconds == 0.5,
                "Round-tripped native instrument zones should preserve SFZ round-robin and release metadata.");
        requireCrossfadeEquals(roundTripInstrument.instrument.zones.at(0).velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Round-tripped projected first instrument zone crossfade metadata");
        requireCrossfadeEquals(roundTripInstrument.instrument.zones.at(45).velocityCrossfade,
                               25,
                               60,
                               61,
                               84,
                               "Round-tripped projected second instrument layer crossfade metadata");
        requireCrossfadeRuntimeEquals(roundTripInstrument.instrument.zones.at(0).velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      roundTripInstrument.instrument.zones.at(45).id,
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Round-tripped projected first instrument runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(roundTripInstrument.instrument.zones.at(45).velocityCrossfadeRuntime,
                                      25,
                                      84,
                                      roundTripInstrument.instrument.zones.at(0).id,
                                      roundTripInstrument.instrument.zones.at(90).id,
                                      25,
                                      60,
                                      61,
                                      84,
                                      "Round-tripped projected second instrument runtime crossfade metadata");

        RuntimeStreamContainerModel streamContainer;
        streamContainer.schemaName = "drs.streamContainer";
        streamContainer.schemaVersion = 1;
        streamContainer.containerId = "projection-test-stream";
        streamContainer.pageSizeBytes = 4096;
        streamContainer.payloadEncoding = "decoded-float32";
        for (std::size_t index = 0; index < instrument.zones.size(); ++index)
        {
            RuntimeStreamSampleDefinition sample;
            sample.sampleId = "sample-" + std::to_string(index + 1);
            sample.sourcePath = instrument.zones.at(index).samplePath;
            sample.formatName = "flac";
            sample.role = "sfz-region-sample";
            sample.sampleRate = 48000.0;
            sample.frameCount = 1;
            sample.channelCount = 1;
            sample.payloadOffsetBytes = static_cast<std::uint64_t>(index + 1);
            streamContainer.samples.push_back(std::move(sample));
        }

        RuntimeVoiceAllocationRequest request;
        request.midiNote = 29;
        request.velocity = 32;
        request.articulationId = "sustain";

        request.voiceId = 1;
        const auto firstRoute = resolveRuntimeVoiceRoute(instrument, streamContainer, request);
        request.voiceId = 2;
        const auto secondRoute = resolveRuntimeVoiceRoute(instrument, streamContainer, request);
        request.voiceId = 3;
        const auto thirdRoute = resolveRuntimeVoiceRoute(instrument, streamContainer, request);
        require(firstRoute.resolved && secondRoute.resolved && thirdRoute.resolved,
                "Projected SFZ round-robin routes should remain resolvable in the native runtime model.");
        require(firstRoute.zoneId != secondRoute.zoneId
                    && secondRoute.zoneId != thirdRoute.zoneId
                    && firstRoute.zoneId != thirdRoute.zoneId,
                "Projected SFZ round-robin routing should select a different region for the first three voice starts.");

        std::cout << "Sprint 3.1.4 SFZ projection tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.4 SFZ projection tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
