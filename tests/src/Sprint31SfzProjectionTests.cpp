#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
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

std::string joinIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    std::ostringstream stream;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index > 0)
            stream << " | ";
        stream << issues[index];
    }
    return stream.str();
}

std::size_t countDistinctGroupIds(const std::vector<drs::engine::RuntimeProjectZoneDefinition>& zones)
{
    std::set<std::string> groupIds;
    for (const auto& zone : zones)
    {
        if (!zone.groupId.empty())
            groupIds.insert(zone.groupId);
    }
    return groupIds.size();
}

const drs::engine::RuntimeProjectGroupDefinition* findGroupById(
    const std::vector<drs::engine::RuntimeProjectGroupDefinition>& groups,
    const std::string& groupId)
{
    const auto iterator = std::find_if(groups.begin(),
                                       groups.end(),
                                       [&](const drs::engine::RuntimeProjectGroupDefinition& group)
                                       {
                                           return group.id == groupId;
                                       });
    return iterator == groups.end() ? nullptr : &(*iterator);
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

fs::path resolveFirstFixturePath()
{
    return resolveFixturePath(
        "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");
}

fs::path resolveFirstSamplePath(const fs::path& fixturePath)
{
    constexpr std::array extensions { ".flac", ".wav", ".aif", ".aiff" };
    for (const auto& entry : fs::recursive_directory_iterator(fixturePath.parent_path()))
    {
        if (!entry.is_regular_file())
            continue;

        const auto extension = entry.path().extension().generic_string();
        if (std::find(extensions.begin(), extensions.end(), extension) != extensions.end())
            return entry.path();
    }

    throw std::runtime_error("Could not locate a sample asset next to the first SFZ fixture.");
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

drs::engine::RuntimeProjectModel makeBlankPhase5Project(const fs::path& fixturePath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 5;
    project.projectId = "sprint31.sfz-projection-schema5";
    project.displayName = "Sprint 3.1.4 Projection Schema 5";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / "projection-test-schema5.drstrm").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 4;
    return project;
}

drs::engine::RuntimeProjectModel makeBlankPhase6Project(const fs::path& fixturePath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "sprint31.sfz-projection-schema6";
    project.displayName = "Sprint 3.1.4 Projection Schema 6";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / "projection-test-schema6.drstrm").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
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
        const auto blankPhase5Project = makeBlankPhase5Project(fixturePath);
        const auto blankPhase6Project = makeBlankPhase6Project(fixturePath);
        const auto analysis = analyzeSfzImportDocument(fixturePath.generic_string());
        const auto projection = projectSfzImportAnalysis(blankProject, analysis);
        const auto phase5Projection = projectSfzImportAnalysis(blankPhase5Project, analysis);
        const auto phase6Projection = projectSfzImportAnalysis(blankPhase6Project, analysis);

        require(projection.projected, "Sprint 3.1.4 should project the first SFZ fixture into native content.");
        require(projection.playable, "Sprint 3.1.4 should keep the first SFZ fixture playable after projection.");
        require(projection.lossy, "The first SFZ fixture should still require lossy-review notes in Sprint 3.1.4.");
        require(!projection.blocking, "The first SFZ fixture should not block projection when every sample resolves.");
        require(projection.sampleSources.size() == 195,
                "The first SFZ fixture should still project its 195 unique sample sources.");
        require(projection.zones.size() == 225,
                "The first SFZ fixture should still create one projected zone per region.");
        require(projection.semanticAnalyzedRegionCount == projection.zones.size()
                    && projection.unsafeUnconditionalRegionCount == 0
                    && projection.unsafeUnconditionalRegionDocumentOrders.empty()
                    && projection.omittedUnsafeRegionCount == 0
                    && projection.omittedRegionSummaries.empty(),
                "Projection should carry semantic safety metadata without changing safe fixture zones.");
        require(!projection.projectNotes.empty(),
                "Sprint 3.1.4 should persist at least one project-level SFZ provenance note.");
        require(!projection.authoringNotes.empty(),
                "Sprint 3.1.4 should persist creator-facing SFZ compatibility notes.");
        require(phase5Projection.projected && phase5Projection.playable,
                "Sprint 3.1.4 should also project the first SFZ fixture into a fresh schema 5 project. Issues: "
                    + joinIssues(phase5Projection.issues));
        require(phase5Projection.zones.size() == projection.zones.size()
                    && phase5Projection.sampleSources.size() == projection.sampleSources.size(),
                "Schema 5 SFZ projection should produce the same zone and sample counts as the legacy authoring baseline.");
        require(phase5Projection.issues.empty(),
                "Schema 5 SFZ projection should not report legacy round-robin migration failures.");
        require(phase6Projection.projected && phase6Projection.playable,
                "Sprint 3.1.4 should also project the first SFZ fixture into a fresh schema 6 project. Issues: "
                    + joinIssues(phase6Projection.issues));
        require(phase6Projection.zones.size() == projection.zones.size()
                    && phase6Projection.sampleSources.size() == projection.sampleSources.size(),
                "Schema 6 SFZ projection should produce the same zone and sample counts as the legacy authoring baseline.");
        require(phase6Projection.issues.empty(),
                "Schema 6 SFZ projection should synthesize any required articulation metadata.");
        require(projection.masterGainDb == 6.0,
                "Stage 1 should surface the SFZ master volume on the projection contract.");
        require(projection.groups.size() == countDistinctGroupIds(projection.zones),
                "Stage 1 should surface one projected group per distinct imported groupId.");

        const auto& firstZone = projection.zones.at(0);
        const auto& secondZone = projection.zones.at(1);
        const auto& thirdZone = projection.zones.at(2);
        const auto& secondLayerFirstZone = projection.zones.at(45);
        const auto* firstProjectedGroup = findGroupById(projection.groups, firstZone.groupId);
        require(firstZone.roundRobinLength == 3
                    && secondZone.roundRobinLength == 3
                    && thirdZone.roundRobinLength == 3,
                "The first projected SFZ regions should preserve 3-way round-robin depth.");
        require(firstZone.roundRobinPosition == 1
                    && secondZone.roundRobinPosition == 2
                    && thirdZone.roundRobinPosition == 3,
                "The first projected SFZ regions should preserve round-robin positions 1, 2, and 3.");
        require(firstZone.roundRobin.has_value()
                    && secondZone.roundRobin.has_value()
                    && thirdZone.roundRobin.has_value()
                    && firstZone.roundRobin->poolId == secondZone.roundRobin->poolId
                    && secondZone.roundRobin->poolId == thirdZone.roundRobin->poolId
                    && firstZone.roundRobin->slotCount == 3
                    && secondZone.roundRobin->slotIndex == 2
                    && thirdZone.roundRobin->mode == RoundRobinMode::sequential,
                "The first projected SFZ regions should now share one explicit sequential round-robin pool descriptor.");
        require(firstZone.gainDb == 0.0,
                "Stage 2 should stop flattening inherited master gain into projected zone gain.");
        require(firstProjectedGroup != nullptr
                    && firstProjectedGroup->displayName == firstZone.groupId
                    && firstProjectedGroup->displayOrder == 0
                    && firstProjectedGroup->auditionAnchorZoneId == firstZone.id
                    && firstProjectedGroup->gainDb == 0.0,
                "Stage 2 should keep projecting a deterministic authored group for the first imported zone.");
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

        const auto firstSamplePath = resolveFirstSamplePath(fixturePath);
        const auto scopedVolumeTempDirectory = fs::temp_directory_path() / "drs-sprint31-sfz-stage1";
        const auto scopedVolumeFixturePath = scopedVolumeTempDirectory / "scoped-volume-stage1.sfz";
        writeTextFile(scopedVolumeFixturePath,
                      "<master>\n"
                      "volume=2\n"
                      "\n"
                      "<group> volume=-3 lovel=1 hivel=63\n"
                      "<region> sample=" + firstSamplePath.generic_string()
                          + " lokey=C4 hikey=C4 pitch_keycenter=C4 volume=1\n"
                            "\n"
                            "<group> volume=-6 lovel=64 hivel=127\n"
                            "<region> sample="
                          + firstSamplePath.generic_string()
                          + " lokey=C4 hikey=C4 pitch_keycenter=C4 volume=4\n");
        const auto scopedVolumeProjection =
            projectSfzImportDocument(blankPhase6Project, scopedVolumeFixturePath.generic_string());
        require(scopedVolumeProjection.projected && scopedVolumeProjection.playable,
                "Stage 1 should keep the scoped-volume fixture projectable and playable.");
        require(scopedVolumeProjection.masterGainDb == 2.0,
                "Stage 1 should capture master volume on the projection result.");
        require(scopedVolumeProjection.groups.size() == 1,
                "Stage 2 should keep velocity-layered scoped-volume content inside one projected group.");
        require(scopedVolumeProjection.zones.size() == 2,
                "Stage 2 should still create one zone per scoped-volume region.");
        require(scopedVolumeProjection.zones[0].groupId == scopedVolumeProjection.zones[1].groupId,
                "Stage 2 should not split projected groups by velocity layering.");
        const auto* scopedVolumeGroup =
            findGroupById(scopedVolumeProjection.groups, scopedVolumeProjection.zones[0].groupId);
        require(scopedVolumeGroup != nullptr
                    && scopedVolumeGroup->gainDb == 0.0
                    && scopedVolumeGroup->displayOrder == 0
                    && scopedVolumeGroup->auditionAnchorZoneId == scopedVolumeProjection.zones[0].id,
                "Stage 2 should normalize mixed group-local gain into zone gain when velocity layers share one projected group.");
        require(scopedVolumeProjection.zones[0].gainDb == -2.0
                    && scopedVolumeProjection.zones[1].gainDb == -2.0,
                "Stage 2 should preserve net audible gain after collapsing velocity-layered groups.");

        const auto velocityLayerTempDirectory = fs::temp_directory_path() / "drs-sprint31-sfz-velocity-layers";
        const auto velocityLayerFixturePath = velocityLayerTempDirectory / "velocity-layer-groups.sfz";
        writeTextFile(velocityLayerFixturePath,
                      "<group>\n"
                      "<region> sample=" + firstSamplePath.generic_string()
                          + " lokey=C4 hikey=C4 lovel=1 hivel=63 pitch_keycenter=C4\n"
                            "<region> sample="
                          + firstSamplePath.generic_string()
                          + " lokey=C4 hikey=C4 lovel=64 hivel=127 pitch_keycenter=C4\n");
        const auto velocityLayerProjection =
            projectSfzImportDocument(blankPhase6Project, velocityLayerFixturePath.generic_string());
        require(velocityLayerProjection.projected && velocityLayerProjection.playable,
                "Stage 2 should keep a simple velocity-layer fixture projectable and playable.");
        require(velocityLayerProjection.groups.size() == 1
                    && velocityLayerProjection.zones.size() == 2,
                "Stage 2 should project one DRS group with two zones for a single SFZ group with two velocity layers.");
        require(velocityLayerProjection.zones[0].groupId == velocityLayerProjection.zones[1].groupId,
                "Stage 2 should preserve velocity layering at the zone level instead of splitting projected groups.");
        require(velocityLayerProjection.zones[0].velocityLow == 1
                    && velocityLayerProjection.zones[0].velocityHigh == 63
                    && velocityLayerProjection.zones[1].velocityLow == 64
                    && velocityLayerProjection.zones[1].velocityHigh == 127,
                "Stage 2 should preserve projected zone velocity ranges when collapsing velocity-layered groups.");

        AuthoringSession phase5Session(blankPhase5Project);
        const auto phase5ApplyResult = applySfzImportProjection(
            phase5Session,
            phase5Projection,
            "Import Sprint 3.1.4 SFZ fixture into schema 5 project");
        require(phase5ApplyResult.applied,
                "Sprint 3.1.4 should apply projected SFZ content into a fresh schema 5 project.");
        require(phase5Session.getProject().schemaVersion == 5
                    && phase5Session.getProject().authoring.schemaVersion == 4,
                "Applying SFZ content into a schema 5 project should preserve the current project schema versions.");
        require(std::abs(phase5Session.getProject().authoring.masterGainDb - phase5Projection.masterGainDb) < 1.0e-9,
                "Applying SFZ content into a schema 5 project should commit imported master gain.");
        require(phase5Session.getProject().authoring.zones.size() == phase5Projection.zones.size(),
                "Applying SFZ content into a schema 5 project should append every projected zone.");
        require(phase5Session.getProject().authoring.groups.size()
                    == phase5Projection.groups.size(),
                "Applying SFZ content into a schema 5 project should commit projected authored groups atomically.");
        require(phase5Session.getProject().authoring.selectedGroupId
                    == phase5Session.getProject().authoring.zones.front().groupId,
                "Applying SFZ content into a schema 5 project should align selectedGroupId with the imported selection.");
        const auto* phase5FirstAppliedGroup =
            findGroupById(phase5Session.getProject().authoring.groups, phase5Projection.groups.front().id);
        require(phase5FirstAppliedGroup != nullptr
                    && phase5FirstAppliedGroup->gainDb == phase5Projection.groups.front().gainDb
                    && phase5FirstAppliedGroup->displayName == phase5Projection.groups.front().displayName,
                "Applying SFZ content into a schema 5 project should preserve projected group gain and naming.");

        AuthoringSession phase6Session(blankPhase6Project);
        const auto phase6ApplyResult = applySfzImportProjection(
            phase6Session,
            phase6Projection,
            "Import Sprint 3.1.4 SFZ fixture into schema 6 project");
        require(phase6ApplyResult.applied,
                "Sprint 3.1.4 should apply projected SFZ content into a fresh schema 6 project.");
        require(phase6Session.getProject().schemaVersion == 6
                    && phase6Session.getProject().authoring.schemaVersion == 5,
                "Applying SFZ content into a schema 6 project should preserve the current project schema versions.");
        require(std::abs(phase6Session.getProject().authoring.masterGainDb - phase6Projection.masterGainDb) < 1.0e-9,
                "Applying SFZ content into a schema 6 project should commit imported master gain.");
        require(phase6Session.getProject().authoring.groups.size() == phase6Projection.groups.size(),
                "Applying SFZ content into a schema 6 project should commit projected authored groups atomically.");
        require(phase6Session.getProject().authoring.articulations.size() == 1,
                "Applying SFZ content into a schema 6 project should synthesize one explicit articulation for this sustain-only fixture.");
        require(phase6Session.getProject().authoring.articulations.front().id == "sustain"
                    && phase6Session.getProject().authoring.articulations.front().isDefault,
                "Schema 6 SFZ projection should synthesize a default sustain articulation for the imported zones.");

        AuthoringSession scopedVolumeSession(blankPhase6Project);
        const auto scopedVolumeApplyResult = applySfzImportProjection(
            scopedVolumeSession,
            scopedVolumeProjection,
            "Import scoped-volume Stage 3 fixture into schema 6 project");
        require(scopedVolumeApplyResult.applied,
                "Stage 3 should apply the scoped-volume fixture through the authoring session.");
        require(std::abs(scopedVolumeSession.getProject().authoring.masterGainDb - 2.0) < 1.0e-9,
                "Stage 3 should commit the imported master gain in the authoring project.");
        require(scopedVolumeSession.getProject().authoring.groups.size() == scopedVolumeProjection.groups.size()
                    && scopedVolumeSession.getProject().authoring.zones.size() == scopedVolumeProjection.zones.size(),
                "Stage 3 should commit imported groups and zones atomically.");
        const auto* appliedScopedVolumeGroup =
            findGroupById(scopedVolumeSession.getProject().authoring.groups, scopedVolumeProjection.groups[0].id);
        require(appliedScopedVolumeGroup != nullptr
                    && appliedScopedVolumeGroup->gainDb == 0.0,
                "Stage 3 should preserve the normalized projected group gain through apply.");
        require(scopedVolumeSession.getProject().authoring.zones[0].gainDb == -2.0
                    && scopedVolumeSession.getProject().authoring.zones[1].gainDb == -2.0,
                "Stage 3 should preserve net audible zone gain through apply after collapsing velocity-layered groups.");
        require(scopedVolumeSession.getProject().authoring.selectedZoneId
                    == scopedVolumeSession.getProject().authoring.zones.front().id
                    && scopedVolumeSession.getProject().authoring.selectedGroupId
                        == scopedVolumeSession.getProject().authoring.zones.front().groupId,
                "Stage 3 should settle selected zone and group coherently after import.");
        const auto scopedUndoResult = scopedVolumeSession.undo();
        require(scopedUndoResult.applied,
                "Stage 3 scoped-volume import should be undoable.");
        require(std::abs(scopedVolumeSession.getProject().authoring.masterGainDb) < 1.0e-9
                    && scopedVolumeSession.getProject().authoring.groups.empty()
                    && scopedVolumeSession.getProject().authoring.zones.empty(),
                "Undo should roll back imported master gain, groups, and zones together.");
        const auto scopedRedoResult = scopedVolumeSession.redo();
        require(scopedRedoResult.applied,
                "Stage 3 scoped-volume import should be redoable.");
        require(std::abs(scopedVolumeSession.getProject().authoring.masterGainDb - 2.0) < 1.0e-9
                    && scopedVolumeSession.getProject().authoring.groups.size() == 1
                    && scopedVolumeSession.getProject().authoring.zones.size() == 2,
                "Redo should restore imported master gain, groups, and zones together.");

        AuthoringSession session(blankProject);
        const auto applyResult = applySfzImportProjection(session, projection, "Import Sprint 3.1.4 SFZ fixture");
        require(applyResult.applied, "Sprint 3.1.4 should apply the projected SFZ content through an undo-safe authoring commit.");
        require(session.getProject().sampleSources.size() == 195,
                "Applying the first SFZ fixture should append the projected sample sources.");
        require(session.getProject().authoring.zones.size() == 225,
                "Applying the first SFZ fixture should append the projected zones.");
        require(std::abs(session.getProject().authoring.masterGainDb - projection.masterGainDb) < 1.0e-9,
                "Applying the first SFZ fixture should commit imported master gain even on legacy schema projects.");
        require(!session.getProject().notes.empty() && !session.getProject().authoring.notes.empty(),
                "Applying the first SFZ fixture should persist both project and authoring SFZ notes.");
        require(session.getDocumentState().revision == 1 && session.getDocumentState().undoDepth == 1,
                "Applying the first SFZ fixture should create exactly one undoable authoring revision.");

        const auto undoResult = session.undo();
        require(undoResult.applied, "The projected SFZ import should be undoable.");
        require(session.getProject().sampleSources.empty()
                    && session.getProject().authoring.zones.empty()
                    && std::abs(session.getProject().authoring.masterGainDb) < 1.0e-9
                    && session.getProject().notes.empty()
                    && session.getProject().authoring.notes.empty(),
                "Undo should fully remove the projected SFZ content and saved notes.");

        const auto redoResult = session.redo();
        require(redoResult.applied, "The projected SFZ import should be redoable.");
        require(session.getProject().sampleSources.size() == 195
                    && session.getProject().authoring.zones.size() == 225
                    && std::abs(session.getProject().authoring.masterGainDb - projection.masterGainDb) < 1.0e-9,
                "Redo should fully restore the projected SFZ content and imported master gain.");

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
        require(roundTripProject.project.authoring.zones.at(0).roundRobin.has_value()
                    && roundTripProject.project.authoring.zones.at(0).roundRobin->slotCount == 3
                    && roundTripProject.project.authoring.zones.at(0).roundRobin->slotIndex == 1,
                "Projected SFZ zones should preserve explicit round-robin descriptors through project round-tripping.");
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
        require(roundTripInstrument.instrument.zones.at(0).roundRobin.has_value()
                    && roundTripInstrument.instrument.zones.at(0).roundRobin->slotCount == 3
                    && roundTripInstrument.instrument.zones.at(2).roundRobin->slotIndex == 3,
                "Round-tripped native instrument zones should preserve explicit round-robin descriptors.");
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

        const auto unsupportedSamplePath = resolveFirstSamplePath(fixturePath).lexically_normal();
        const auto unsupportedFixtureDirectory =
            fs::temp_directory_path() / "drs-sprint31-sfz-projection-fallback";
        const auto unsupportedFixturePath = unsupportedFixtureDirectory / "unsupported-topology.sfz";
        writeTextFile(
            unsupportedFixturePath,
            "<group>\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "lovel=25\n"
            "hivel=84\n"
            "xfin_lovel=25\n"
            "xfin_hivel=60\n"
            "<region>\n"
            "sample=" + unsupportedSamplePath.generic_string() + "\n");

        const auto unsupportedAnalysis = analyzeSfzImportDocument(unsupportedFixturePath.generic_string());
        require(unsupportedAnalysis.analyzed && unsupportedAnalysis.report.available,
                "Unsupported topology should still analyze for projection fallback coverage.");
        const auto unsupportedProjection = projectSfzImportAnalysis(blankProject, unsupportedAnalysis);
        require(unsupportedProjection.projected && unsupportedProjection.playable
                    && unsupportedProjection.lossy && !unsupportedProjection.blocking,
                "Unsupported crossfade topology should degrade into a playable reviewed projection.");
        require(unsupportedProjection.zones.size() == 1,
                "Unsupported topology fallback should still create one native zone.");
        require(!hasAnyVelocityCrossfadeValue(unsupportedProjection.zones.front().velocityCrossfade),
                "Unsupported topology fallback should strip native crossfade metadata from the projected zone.");
        require(unsupportedProjection.zones.front().velocityLow == 25
                    && unsupportedProjection.zones.front().velocityHigh == 84,
                "Unsupported topology fallback should preserve the plain velocity window.");

        AuthoringSession fallbackSession(blankProject);
        const auto fallbackApplyResult = applySfzImportProjection(
            fallbackSession,
            unsupportedProjection,
            "Import unsupported SFZ topology as fallback");
        require(fallbackApplyResult.applied,
                "Unsupported topology fallback should remain applyable after review.");
        require(fallbackSession.getProject().authoring.zones.size() == 1
                    && !hasAnyVelocityCrossfadeValue(
                        fallbackSession.getProject().authoring.zones.front().velocityCrossfade),
                "Unsupported topology fallback should persist as a plain native zone.");

        const auto soundSafeFixturePath = unsupportedFixtureDirectory / "sound-safe-omission.sfz";
        writeTextFile(
            soundSafeFixturePath,
            "<control>\n"
            "set_cc23=0\n"
            "<group>\n"
            "volume=-2\n"
            "group_volume=-3\n"
            "tune=25\n"
            "amp_veltrack=50\n"
            "<region>\n"
            "sample=" + unsupportedSamplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=60\n"
            "hikey=60\n"
            "<group>\n"
            "locc23=1\n"
            "<region>\n"
            "sample=" + unsupportedSamplePath.generic_string() + "\n"
            "pitch_keycenter=61\n"
            "lokey=61\n"
            "hikey=61\n"
            "<group>\n"
            "trigger=release\n"
            "pitch_keytrack=0\n"
            "<region>\n"
            "sample=" + unsupportedSamplePath.generic_string() + "\n"
            "pitch_keycenter=60\n"
            "lokey=62\n"
            "hikey=62\n"
            "<group>\n"
            "locc11=64\n"
            "on_locc23=1\n"
            "on_hicc23=63\n"
            "<region>\n"
            "sample=" + unsupportedSamplePath.generic_string() + "\n"
            "pitch_keycenter=63\n"
            "lokey=63\n"
            "hikey=63\n");
        const auto soundSafeProjection = projectSfzImportDocument(
            blankProject, soundSafeFixturePath.generic_string());
        require(soundSafeProjection.projected && soundSafeProjection.playable
                    && !soundSafeProjection.lossy && !soundSafeProjection.blocking,
                "Native controller and release semantics should remain a playable lossless projection.");
        require(soundSafeProjection.semanticAnalyzedRegionCount == 4
                    && soundSafeProjection.unsafeUnconditionalRegionCount == 0
                    && soundSafeProjection.omittedUnsafeRegionCount == 0,
                "Controller and release regions should no longer be omitted.");
        require(soundSafeProjection.zones.size() == 4
                    && soundSafeProjection.controllerDefaults.size() == 1
                    && soundSafeProjection.controllerDefaults.front().controllerNumber == 23,
                "Projection should retain all zones and the authored CC23 default.");
        require(soundSafeProjection.groups.front().gainDb == -5.0
                    && soundSafeProjection.zones.front().fineTuneCents == 25.0
                    && soundSafeProjection.zones.front().amplitudeVelocityTracking == 50.0,
                "Projection must preserve inherited group_volume, fine tuning, and velocity tracking.");
        require(soundSafeProjection.zones[1].controllerConditions.size() == 1
                    && soundSafeProjection.zones[1].controllerConditions.front().controllerNumber == 23
                    && soundSafeProjection.zones[2].performance.event == PerformanceEventKind::release
                    && soundSafeProjection.zones[2].performance.pitchSource
                        == PerformancePitchSource::eventKeyFixedPitch
                    && soundSafeProjection.zones[2].triggerMode == ZoneTriggerMode::oneShot,
                "Projection should preserve controller eligibility, release triggers, and zero pitch tracking.");
        require(soundSafeProjection.zones[3].performance.event == PerformanceEventKind::controllerChange
                    && soundSafeProjection.zones[3].performance.triggerControllerNumber == 23
                    && soundSafeProjection.zones[3].controllerConditions.size() == 2
                    && soundSafeProjection.zones[3].controllerConditions.front().controllerNumber == 11,
                "Projection must represent the controller trigger explicitly without reordering static conditions.");

        AuthoringSession soundSafeSession(blankProject);
        const auto soundSafeApply = applySfzImportProjection(
            soundSafeSession, soundSafeProjection, "Import only sound-safe SFZ zones");
        require(soundSafeApply.applied
                    && soundSafeSession.getProject().authoring.zones.size() == 4
                    && soundSafeSession.getProject().authoring.controllerDefaults.size() == 1,
                "Reviewed sound-safe apply should mutate the project with retained zones only.");

        const auto salamanderPath = resolveFixturePath(
            "DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_daw/Accurate-SalamanderGrandPiano_flat.Recommended.sfz");
        const auto salamanderProjection = projectSfzImportDocument(
            makeBlankPhase2Project(salamanderPath), salamanderPath.generic_string());
        require(salamanderProjection.projected && salamanderProjection.playable,
                "Accurate Salamander sound-safe projection should remain playable.");
        require(salamanderProjection.semanticAnalyzedRegionCount == 1704
                    && salamanderProjection.unsafeUnconditionalRegionCount == 4
                    && salamanderProjection.omittedUnsafeRegionCount == 4
                    && salamanderProjection.zones.size() == 1700
                    && salamanderProjection.sampleSources.size() == 637,
                "Accurate Salamander Phase 3 projection should retain controller/release layers and omit only four random pedal regions; sources="
                    + std::to_string(salamanderProjection.sampleSources.size()));

        const auto hasControllerCondition = [](const RuntimeProjectZoneDefinition& zone,
                                               const int controller)
        {
            return std::any_of(zone.controllerConditions.begin(), zone.controllerConditions.end(),
                               [&](const RuntimeControllerCondition& condition)
                               { return condition.controllerNumber == controller; });
        };
        const auto resonanceZoneCount = std::count_if(
            salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
            [&](const RuntimeProjectZoneDefinition& zone)
            {
                return zone.performance.event == PerformanceEventKind::noteOn
                    && hasControllerCondition(zone, 23) && hasControllerCondition(zone, 64);
            });
        const auto sampledReleaseZoneCount = std::count_if(
            salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
            [&](const RuntimeProjectZoneDefinition& zone)
            {
                return zone.performance.event == PerformanceEventKind::release
                    && hasControllerCondition(zone, 20) && hasControllerCondition(zone, 64);
            });
        const auto hammerZoneCount = std::count_if(
            salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
            [&](const RuntimeProjectZoneDefinition& zone)
            {
                return zone.performance.event == PerformanceEventKind::release
                    && hasControllerCondition(zone, 21) && hasControllerCondition(zone, 64);
            });
        const auto retainedPedalActionCount = std::count_if(
            salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
            [](const RuntimeProjectZoneDefinition& zone)
            {
                return zone.performance.event == PerformanceEventKind::pedalDown
                    || zone.performance.event == PerformanceEventKind::pedalUp;
            });
        require(resonanceZoneCount == 135 && sampledReleaseZoneCount == 69
                    && hammerZoneCount == 88 && retainedPedalActionCount == 0,
                "Phase 4 must retain 135 gated resonance and 157 gated release/hammer zones while omitting all four random pedal variants.");
        require(std::all_of(
                    salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
                    [&](const RuntimeProjectZoneDefinition& zone)
                    {
                        return !(zone.performance.event == PerformanceEventKind::release
                                 && hasControllerCondition(zone, 21))
                            || zone.performance.pitchSource == PerformancePitchSource::eventKeyFixedPitch;
                    }),
                "Every Salamander hammer-noise route must use event-key eligibility without transposing its sample.");

        std::vector<const RuntimeProjectZoneDefinition*> middleCResonanceZones;
        for (const auto& zone : salamanderProjection.zones)
            if (zone.performance.event == PerformanceEventKind::noteOn
                && zone.keyLow <= 60 && zone.keyHigh >= 60
                && hasControllerCondition(zone, 23) && hasControllerCondition(zone, 64))
                middleCResonanceZones.push_back(&zone);
        require(middleCResonanceZones.size() == 2,
                "Enabled Salamander middle-C resonance must add exactly its two intended sympathetic routes.");
        require(std::all_of(
                    middleCResonanceZones.begin(), middleCResonanceZones.end(),
                    [&](const RuntimeProjectZoneDefinition* zone)
                    {
                        const auto* group = findGroupById(salamanderProjection.groups, zone->groupId);
                        return group != nullptr
                            && std::abs(group->gainDb + zone->gainDb + 6.0) < 0.000001;
                    })
                    && std::any_of(salamanderProjection.zones.begin(), salamanderProjection.zones.end(),
                                   [&](const RuntimeProjectZoneDefinition& zone)
                                   {
                                       return hasControllerCondition(zone, 23)
                                           && zone.fineTuneCents != 0.0;
                                   }),
                "Salamander resonance must retain its -6 dB middle-register group attenuation and authored fine tuning.");
        const auto omittedRandomRegionCount = std::accumulate(
            salamanderProjection.omittedRegionSummaries.begin(),
            salamanderProjection.omittedRegionSummaries.end(), std::size_t { 0 },
            [](const std::size_t count, const SfzImportOmittedRegionSummary& summary)
            {
                return count + (summary.dependencyKind == SfzImportSemanticDependencyKind::randomPolicy
                    ? summary.affectedRegionCount : 0);
            });
        require(omittedRandomRegionCount == 4
                    && std::all_of(salamanderProjection.omittedRegionSummaries.begin(),
                                   salamanderProjection.omittedRegionSummaries.end(),
                                   [](const SfzImportOmittedRegionSummary& summary)
                                   {
                                       return !summary.feature.empty() && !summary.sourcePath.empty()
                                           && summary.firstSourceLineNumber > 0;
                                   }),
                "The sound-safe random policy must report every omitted pedal variant with feature and source context.");

        const auto pedalFixturePath = unsupportedFixtureDirectory / "phase4-pedal-actions.sfz";
        writeTextFile(
            pedalFixturePath,
            "<control>\nset_cc22=0\n"
            "<master> locc22=1\n"
            "<group> on_locc64=126 on_hicc64=127\n"
            "<region> sample=" + unsupportedSamplePath.generic_string()
                + " pitch_keycenter=60 lokey=60 hikey=60\n"
                  "<group> on_locc64=0 on_hicc64=1\n"
                  "<region> sample=" + unsupportedSamplePath.generic_string()
                + " pitch_keycenter=60 lokey=60 hikey=60\n");
        const auto pedalProjection = projectSfzImportDocument(
            blankPhase6Project, pedalFixturePath.generic_string());
        require(pedalProjection.projected && pedalProjection.playable
                    && pedalProjection.zones.size() == 2
                    && pedalProjection.zones[0].performance.event == PerformanceEventKind::pedalDown
                    && pedalProjection.zones[1].performance.event == PerformanceEventKind::pedalUp
                    && pedalProjection.zones[0].performance.triggerControllerNumber == 64
                    && pedalProjection.zones[1].performance.triggerControllerNumber == 64,
                "Non-random pedal-action regions must map only to their matching CC64 transitions.");

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
