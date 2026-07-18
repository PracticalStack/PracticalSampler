#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

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

bool containsFinding(const drs::engine::PreparedPlaybackBuildResult& result,
                     drs::engine::PlaybackSnapshotFindingSeverity severity,
                     const std::string& code,
                     const std::string& pathFragment)
{
    for (const auto& finding : result.findings)
    {
        if (finding.severity == severity
            && finding.code == code
            && finding.path.find(pathFragment) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}
} // namespace

int main()
{
    try
    {
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before prepared playback tests run.");

        const auto referenceManifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded, "Phase 1 reference manifest must load before prepared playback tests run.");

        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded, "Phase 1 reference stream must load before prepared playback tests run.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        drs::engine::PreparedPlaybackService preparedService;

        const auto firstSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        require(firstSnapshotRequest.accepted, "Initial playback snapshot request should be accepted.");
        const auto firstSnapshot = snapshotBuilder.buildSnapshot(firstSnapshotRequest, phase2Project.project);
        require(firstSnapshot.built, "Initial playback snapshot should build successfully.");

        const auto firstPreparedRequest = preparedService.requestBuild(firstSnapshot);
        require(firstPreparedRequest.accepted, "Prepared playback request should be accepted for a valid snapshot.");
        require(firstPreparedRequest.snapshotBuildId == firstSnapshot.buildId,
                "Prepared playback request should track the immutable snapshot build identity.");
        require(firstPreparedRequest.requestedDraftRevision == firstSnapshot.requestedDraftRevision,
                "Prepared playback request should track the requested draft revision.");
        require(firstPreparedRequest.activationRequested == firstSnapshot.activationRequested,
                "Prepared playback request should preserve whether activation was requested.");
        require(firstPreparedRequest.cancellationId == firstPreparedRequest.buildId,
                "Prepared playback request should seed cancellation identity from its build identity.");
        require(firstPreparedRequest.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::preparing,
                "Accepted prepared playback requests should begin in the preparing state.");
        const auto firstPrepared = preparedService.prepare(firstPreparedRequest, firstSnapshot, referenceStream);
        require(firstPrepared.built, "Prepared playback should build from the reference snapshot.");
        require(firstPrepared.activationEligible, "Prepared playback should remain activation-eligible for valid content.");
        require(firstPrepared.buildId == firstPreparedRequest.buildId,
                "Prepared playback result should preserve the request build identity.");
        require(firstPrepared.snapshotBuildId == firstPreparedRequest.snapshotBuildId,
                "Prepared playback result should preserve the immutable snapshot build identity.");
        require(firstPrepared.requestedDraftRevision == firstPreparedRequest.requestedDraftRevision,
                "Prepared playback result should preserve the requested draft revision.");
        require(firstPrepared.activationRequested == firstPreparedRequest.activationRequested,
                "Prepared playback result should preserve whether activation was requested.");
        require(firstPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Successful prepared playback should finish in the ready state.");
        require(firstPrepared.buildDurationMicros > 0,
                "Prepared playback result should report a non-zero build duration.");
        require(firstPrepared.metrics.preparedSampleCount == phase2Project.project.sampleSources.size(),
                "Prepared sample count changed unexpectedly.");
        require(firstPrepared.metrics.preparedZoneCount == phase2Project.project.authoring.zones.size(),
                "Prepared zone count changed unexpectedly.");
        require(firstPrepared.metrics.cacheMissCount == phase2Project.project.sampleSources.size(),
                "First prepared playback build should cold-miss every sample handle.");
        require(firstPrepared.metrics.cacheHitCount == 0,
                "First prepared playback build should not report cache hits.");
        require(!firstPrepared.prepared.preparedContentDigest.empty(),
                "Prepared playback builds must carry a deterministic content digest.");
        require(firstPrepared.prepared.samples[0].ownershipToken.find("cache:") == 0,
                "Prepared sample handles should expose an explicit ownership token.");

        const auto secondSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        const auto secondSnapshot = snapshotBuilder.buildSnapshot(secondSnapshotRequest, phase2Project.project);
        const auto secondPreparedRequest = preparedService.requestBuild(secondSnapshot);
        const auto secondPrepared = preparedService.prepare(secondPreparedRequest, secondSnapshot, referenceStream);
        require(secondPrepared.built, "Repeated prepared playback build should still succeed.");
        require(secondPrepared.prepared.preparedContentDigest == firstPrepared.prepared.preparedContentDigest,
                "Repeated preparation of the same snapshot should produce the same prepared digest.");
        require(secondPrepared.metrics.cacheHitCount == phase2Project.project.sampleSources.size(),
                "Warm prepared playback build should hit the cache for every sample handle.");
        require(secondPrepared.metrics.cacheMissCount == 0,
                "Warm prepared playback build should not cold-miss unchanged sample handles.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        auto editedProject = controller.getProject();
        editedProject.sampleSources[1].path = editedProject.sampleSources[0].path;
        const auto commitResult = controller.commitSnapshot(editedProject,
                                                            "Swap the lead source path to invalidate one prepared key",
                                                            {"sampleSources[1].path"});
        require(commitResult.applied, "Edited project revision should commit before prepared playback rebuild.");

        const auto editedSnapshotRequest = snapshotBuilder.requestBuild(commitResult.documentState.revision, true);
        const auto editedSnapshot = snapshotBuilder.buildSnapshot(editedSnapshotRequest, controller.getProject());
        require(editedSnapshot.built, "Edited snapshot should still build successfully.");
        const auto editedPreparedRequest = preparedService.requestBuild(editedSnapshot);
        const auto editedPrepared = preparedService.prepare(editedPreparedRequest, editedSnapshot, referenceStream);
        require(editedPrepared.built, "Edited prepared playback should still succeed.");
        require(editedPrepared.prepared.preparedContentDigest != firstPrepared.prepared.preparedContentDigest,
                "Changing a sample source path should invalidate the prepared digest.");
        require(editedPrepared.metrics.cacheHitCount == 1,
                "Changing one source path should preserve exactly one cached prepared asset.");
        require(editedPrepared.metrics.cacheMissCount == 1,
                "Changing one source path should invalidate exactly one cached prepared asset.");

        auto invalidProject = phase2Project.project;
        invalidProject.sampleSources[0].path = invalidProject.contentRootPath + "/Samples/does-not-exist.wav";
        const auto invalidSnapshotRequest = snapshotBuilder.requestBuild(3, true);
        const auto invalidSnapshot = snapshotBuilder.buildSnapshot(invalidSnapshotRequest, invalidProject);
        require(!invalidSnapshot.built, "Invalid snapshot should fail before prepared playback begins.");
        const auto rejectedPreparedRequest = preparedService.requestBuild(invalidSnapshot);
        require(!rejectedPreparedRequest.accepted,
                "Prepared playback request must reject failed immutable snapshots.");
        require(rejectedPreparedRequest.snapshotBuildId == invalidSnapshot.buildId,
                "Rejected prepared playback request should still point at the failed snapshot build identity.");
        require(rejectedPreparedRequest.requestedDraftRevision == invalidSnapshot.requestedDraftRevision,
                "Rejected prepared playback request should preserve the failed snapshot draft revision.");
        require(rejectedPreparedRequest.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Rejected prepared playback request should surface the failed lifecycle state.");
        const auto rejectedPrepared = preparedService.prepare(rejectedPreparedRequest, invalidSnapshot, referenceStream);
        require(!rejectedPrepared.built && !rejectedPrepared.activationEligible,
                "Rejected prepared playback result must not become activation-eligible.");
        require(rejectedPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Rejected prepared playback result should remain in the failed lifecycle state.");
        require(containsFinding(rejectedPrepared,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "missing-sample-source-asset",
                                "sampleSources[0].path"),
                "Rejected prepared playback result should preserve the immutable snapshot findings that caused rejection.");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migrated prepared-playback coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before prepared-playback coverage runs.");

        drs::engine::PlaybackSnapshotBuilder migratedSnapshotBuilder;
        drs::engine::PreparedPlaybackService migratedPreparedService;

        const auto migratedSnapshotRequest = migratedSnapshotBuilder.requestBuild(0, true);
        const auto migratedSnapshot = migratedSnapshotBuilder.buildSnapshot(migratedSnapshotRequest, migratedProject.project);
        require(!migratedSnapshot.built,
                "Migrated Phase 1 project should not build an activation-eligible snapshot before imported zones exist.");
        const auto migratedPreparedRequest = migratedPreparedService.requestBuild(migratedSnapshot);
        require(!migratedPreparedRequest.accepted,
                "Prepared playback request must reject migrated snapshots that still lack playable zones.");
        const auto migratedPreparedRejected = migratedPreparedService.prepare(migratedPreparedRequest,
                                                                             migratedSnapshot,
                                                                             referenceStream);
        require(!migratedPreparedRejected.built,
                "Prepared playback must stay rejected while the migrated project has no imported zones.");
        require(migratedPreparedRejected.snapshotBuildId == migratedSnapshot.buildId,
                "Rejected migrated prepared playback should preserve the snapshot build identity.");
        require(migratedPreparedRejected.requestedDraftRevision == migratedSnapshot.requestedDraftRevision,
                "Rejected migrated prepared playback should preserve the requested draft revision.");
        require(migratedPreparedRejected.prepared.draftRevision == migratedSnapshot.snapshot.draftRevision,
                "Rejected migrated prepared playback should still report the snapshot draft revision.");
        require(migratedPreparedRejected.prepared.samples.empty() && migratedPreparedRejected.prepared.zones.empty(),
                "Rejected migrated prepared playback must not fabricate prepared samples or zones.");
        require(containsFinding(migratedPreparedRejected,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Rejected migrated prepared playback should preserve the structured no-playable-zones finding.");
        require(!containsFinding(migratedPreparedRejected,
                                 drs::engine::PlaybackSnapshotFindingSeverity::error,
                                 "no-sample-identities",
                                 "sampleSources"),
                "Rejected migrated prepared playback should preserve the migrated sample identities while zones are missing.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "migrated-import-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "migrated-zone-sine-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Migrated Imported Sine A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto importResult = migratedSession.appendImportedContent({ importedSampleSource },
                                                                        { importedZone },
                                                                        "Import migrated authoring content");
        require(importResult.applied, "Migrated Phase 1 project should accept imported authoring content.");
        require(importResult.documentState.revision == 1,
                "Imported migrated authoring content should advance the document revision.");
        require(migratedSession.getProject().authoring.selectedZoneId == importedZone.id,
                "Imported migrated authoring content should select the imported zone.");

        const auto importedSnapshotRequest = migratedSnapshotBuilder.requestBuild(importResult.documentState.revision, true);
        const auto importedSnapshot = migratedSnapshotBuilder.buildSnapshot(importedSnapshotRequest, migratedSession.getProject());
        require(importedSnapshot.built,
                "Migrated project with imported authoring content should build an immutable snapshot.");
        require(importedSnapshot.snapshot.selectedZoneId == importedZone.id,
                "Imported migrated snapshot should preserve the selected zone.");
        const auto importedPreparedRequest = migratedPreparedService.requestBuild(importedSnapshot);
        require(importedPreparedRequest.accepted,
                "Prepared playback should accept migrated snapshots once imported zones exist.");
        const auto importedPrepared = migratedPreparedService.prepare(importedPreparedRequest, importedSnapshot, referenceStream);
        require(importedPrepared.built && importedPrepared.activationEligible,
                "Prepared playback should succeed for migrated projects once imported authoring content exists.");
        require(importedPrepared.prepared.draftRevision == importResult.documentState.revision,
                "Prepared playback should preserve the imported draft revision.");
        require(importedPrepared.metrics.preparedSampleCount == migratedSession.getProject().sampleSources.size(),
                "Prepared playback should materialize every migrated sample identity after import.");
        require(importedPrepared.metrics.preparedZoneCount == migratedSession.getProject().authoring.zones.size(),
                "Prepared playback should materialize the imported migrated zone.");
        require(importedPrepared.metrics.cacheMissCount == migratedSession.getProject().sampleSources.size(),
                "First successful migrated prepared build should cold-miss every migrated sample handle.");
        require(importedPrepared.metrics.cacheHitCount == 0,
                "First successful migrated prepared build should not report cache hits.");
        require(importedPrepared.prepared.zones.size() == 1,
                "Imported migrated prepared playback should expose exactly one playable zone.");
        require(importedPrepared.prepared.zones[0].zoneId == importedZone.id,
                "Imported migrated prepared playback should preserve the imported zone identity.");

        auto editedImportedZone = *migratedSession.getSelectedZone();
        editedImportedZone.gainDb = 2.5;
        editedImportedZone.pan = -0.2;
        const auto editedZoneResult = migratedSession.updateSelectedZone(editedImportedZone,
                                                                         "Shape imported migrated zone");
        require(editedZoneResult.applied, "Editing the imported migrated zone should commit successfully.");
        require(editedZoneResult.documentState.revision == 2,
                "Editing the imported migrated zone should advance the draft revision.");

        const auto editedImportedSnapshotRequest = migratedSnapshotBuilder.requestBuild(editedZoneResult.documentState.revision,
                                                                                        true);
        const auto editedImportedSnapshot = migratedSnapshotBuilder.buildSnapshot(editedImportedSnapshotRequest,
                                                                                  migratedSession.getProject());
        require(editedImportedSnapshot.built,
                "Edited migrated authoring content should still build an immutable snapshot.");
        require(editedImportedSnapshot.snapshot.contentDigest != importedSnapshot.snapshot.contentDigest,
                "Editing imported migrated authoring content should change the immutable snapshot digest.");
        const auto editedImportedPreparedRequest = migratedPreparedService.requestBuild(editedImportedSnapshot);
        const auto editedImportedPrepared = migratedPreparedService.prepare(editedImportedPreparedRequest,
                                                                           editedImportedSnapshot,
                                                                           referenceStream);
        require(editedImportedPrepared.built,
                "Edited migrated authoring content should still prepare successfully.");
        require(editedImportedPrepared.prepared.preparedContentDigest != importedPrepared.prepared.preparedContentDigest,
                "Editing imported migrated authoring content should change the prepared-playback digest.");
        require(editedImportedPrepared.metrics.cacheHitCount == migratedSession.getProject().sampleSources.size(),
                "Editing zone-only migrated content should reuse every prepared sample handle.");
        require(editedImportedPrepared.metrics.cacheMissCount == 0,
                "Editing zone-only migrated content should not invalidate prepared sample handles.");
        require(editedImportedPrepared.prepared.zones[0].gainDb == editedImportedZone.gainDb
                    && editedImportedPrepared.prepared.zones[0].pan == editedImportedZone.pan,
                "Prepared playback should preserve edited migrated zone normalization values.");

        const auto canceledPrepared = preparedService.cancelBuild(firstPreparedRequest);
        require(canceledPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled prepared playback should report the canceled lifecycle state.");
        require(!canceledPrepared.activationEligible,
                "Canceled prepared playback result must never become activation-eligible.");
        require(canceledPrepared.metrics.cancellationCount == 1,
                "Canceled prepared playback result should increment the cancellation metric.");
        require(canceledPrepared.buildId == firstPreparedRequest.buildId
                    && canceledPrepared.cancellationId == firstPreparedRequest.cancellationId,
                "Canceled prepared playback result should preserve request and cancellation identities.");

        const auto supersedingPreparedRequest = preparedService.requestBuild(secondSnapshot);
        const auto supersededPrepared = preparedService.supersedeBuild(firstPreparedRequest,
                                                                       supersedingPreparedRequest.buildId);
        require(supersededPrepared.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::superseded,
                "Superseded prepared playback should report the superseded lifecycle state.");
        require(!supersededPrepared.activationEligible,
                "Superseded prepared playback result must never become activation-eligible.");
        require(supersededPrepared.cancellationId == supersedingPreparedRequest.buildId,
                "Superseded prepared playback result should point at the replacement build identity.");

        std::cout << "Phase 1 prepared playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 prepared playback tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
