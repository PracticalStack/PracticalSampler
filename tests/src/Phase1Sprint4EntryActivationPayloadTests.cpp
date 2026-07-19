#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct BuildArtifacts
{
    drs::engine::PlaybackSnapshotBuildResult snapshot;
    drs::engine::PreparedPlaybackBuildResult prepared;
};

BuildArtifacts buildArtifacts(drs::engine::PlaybackSnapshotBuilder& snapshotBuilder,
                              drs::engine::PreparedPlaybackService& preparedService,
                              const drs::engine::RuntimeStreamLoadResult& stream,
                              const drs::engine::RuntimeProjectModel& project,
                              std::size_t revision,
                              bool activationRequested)
{
    const auto snapshotRequest = snapshotBuilder.requestBuild(revision, activationRequested);
    require(snapshotRequest.accepted, "EG2 snapshot request should be accepted.");
    auto snapshot = snapshotBuilder.buildSnapshot(snapshotRequest, project);
    require(snapshot.built && snapshot.activationEligible, "EG2 snapshot should be activation eligible.");
    const auto preparedRequest = preparedService.requestBuild(snapshot, stream);
    require(preparedRequest.accepted, "EG2 prepared request should be accepted.");
    auto prepared = preparedService.prepare(preparedRequest, snapshot, stream);
    require(prepared.built && prepared.activationEligible, "EG2 prepared result should be activation eligible.");
    return { std::move(snapshot), std::move(prepared) };
}

bool containsFinding(const drs::engine::DraftPlaybackPreparedRevision& revision,
                     const std::string& code)
{
    for (const auto& finding : revision.findings)
        if (finding.code == code)
            return true;
    return false;
}

bool settleFacade(drs::engine::EngineFacade& facade)
{
    if (!facade.waitForPreparedPlaybackIdle())
        return false;

    for (int pass = 0; pass < 8; ++pass)
        facade.serviceBackgroundWork();
    return true;
}

void runContractLifetimeMatrix(const drs::engine::RuntimeProjectModel& project,
                               const drs::engine::RuntimeStreamLoadResult& stream)
{
    static_assert(std::is_const_v<std::remove_reference_t<
                      decltype(*std::declval<drs::engine::PlaybackActivationPayloadPtr>())>>,
                  "Renderer-facing activation payload must be const.");

    drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
    drs::engine::PreparedPlaybackService preparedService;
    drs::engine::DraftPlaybackContract contract(0);

    const auto previewRequest = contract.requestPreviewBuild();
    auto previewArtifacts = buildArtifacts(snapshotBuilder, preparedService, stream, project, 0, false);
    require(contract.completePreviewBuild(previewRequest.requestId,
                                          previewArtifacts.snapshot,
                                          previewArtifacts.prepared),
            "EG2 preview completion should install an immutable payload.");
    auto previewPayload = contract.getStatus().preview.activationPayload;
    require(previewPayload != nullptr
                && previewPayload->lane == drs::engine::PlaybackActivationLane::preview
                && previewPayload->revision == 0
                && previewPayload->activationEligible
                && previewPayload->lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready
                && previewPayload->snapshot != nullptr
                && previewPayload->prepared != nullptr
                && previewPayload->snapshotContentDigest == previewPayload->snapshot->contentDigest
                && previewPayload->preparedContentDigest == previewPayload->prepared->preparedContentDigest
                && previewPayload->retainedPreparedBytes == previewArtifacts.prepared.metrics.preparedBytes,
            "Preview payload should expose coherent revision, lifecycle, digest, and retained handles.");
    require(!previewPayload->prepared->samples.empty()
                && previewPayload->prepared->samples.front().decodedSampleData != nullptr,
            "Prepared PCM must remain reachable through the immutable payload.");

    const auto previewIdentity = previewPayload.get();
    const auto previewPreparedDigest = previewPayload->preparedContentDigest;
    const auto previewRevision = previewPayload->revision;

    const auto publishRequest = contract.requestPerformanceBuild();
    auto publishArtifacts = buildArtifacts(snapshotBuilder, preparedService, stream, project, 0, true);
    require(contract.completePerformanceBuild(publishRequest.requestId,
                                              publishArtifacts.snapshot,
                                              publishArtifacts.prepared),
            "EG2 publish completion should install a separate immutable payload.");
    auto performancePayload = contract.getStatus().performance.activationPayload;
    require(performancePayload != nullptr && performancePayload.get() != previewIdentity
                && performancePayload->lane == drs::engine::PlaybackActivationLane::performance
                && performancePayload->lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
            "Preview and Performance must retain separate last-known-good payloads.");

    require(contract.setDraftRevision(1), "EG2 failure matrix should advance the draft revision.");
    const auto failedRequest = contract.requestPreviewBuild();
    auto failedArtifacts = buildArtifacts(snapshotBuilder, preparedService, stream, project, 1, false);
    failedArtifacts.prepared.built = false;
    failedArtifacts.prepared.activationEligible = false;
    failedArtifacts.prepared.lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::failed;
    failedArtifacts.prepared.state = "Injected EG2 preparation failure";
    require(contract.completePreviewBuild(failedRequest.requestId,
                                          failedArtifacts.snapshot,
                                          failedArtifacts.prepared),
            "Failed completion should be recorded without replacing the active payload.");
    require(contract.getStatus().preview.activationPayload.get() == previewIdentity
                && contract.getStatus().preview.activationEligible
                && contract.getStatus().preview.revision == previewRevision
                && contract.getStatus().preview.preparedContentDigest == previewPreparedDigest,
            "Failed completion must preserve pointer identity, revision, digest, and eligibility.");

    const auto canceledRequest = contract.requestPreviewBuild();
    require(contract.cancelPreviewBuild(canceledRequest.requestId)
                && contract.getStatus().preview.activationPayload.get() == previewIdentity,
            "Canceled completion must preserve the last-known-good payload.");

    const auto supersededRequest = contract.requestPreviewBuild();
    require(contract.setDraftRevision(2), "EG2 supersede matrix should advance the draft revision.");
    const auto replacementRequest = contract.requestPreviewBuild();
    require(!contract.completePreviewBuild(supersededRequest.requestId,
                                           failedArtifacts.snapshot,
                                           failedArtifacts.prepared)
                && contract.getStatus().preview.activationPayload.get() == previewIdentity,
            "Superseded or stale completion must not replace the active payload.");
    require(contract.cancelPreviewBuild(replacementRequest.requestId),
            "Replacement request should remain cancelable after stale completion rejection.");

    const auto mismatchRequest = contract.requestPreviewBuild();
    auto mismatchArtifacts = buildArtifacts(snapshotBuilder, preparedService, stream, project, 2, false);
    mismatchArtifacts.prepared.snapshotBuildId += 1000;
    require(contract.completePreviewBuild(mismatchRequest.requestId,
                                          mismatchArtifacts.snapshot,
                                          mismatchArtifacts.prepared)
                && contract.getStatus().preview.activationPayload.get() == previewIdentity
                && containsFinding(contract.getStatus().preview, "activation-payload-identity-mismatch"),
            "Mismatched snapshot/prepared identities must preserve the old payload and surface a finding.");

    require(contract.beginDeviceRestart(), "EG2 device restart should begin.");
    require(contract.getStatus().preview.activationPayload.get() == previewIdentity
                && contract.getStatus().performance.activationPayload.get() == performancePayload.get(),
            "Device restart must preserve both immutable payloads.");
    require(contract.completeDeviceRestart(true)
                && contract.getStatus().preview.activationPayload.get() == previewIdentity,
            "Successful device restart must keep the last-known-good payload readable.");

    std::weak_ptr<const drs::engine::PlaybackActivationPayload> previewWeak = previewPayload;
    previewPayload.reset();
    performancePayload.reset();
    contract.closeProject();
    require(contract.getStatus().preview.activationPayload == nullptr
                && contract.getStatus().performance.activationPayload == nullptr
                && previewWeak.expired(),
            "Project close must release product-owned payloads on the non-audio owner.");
}

void runFacadeRetentionMatrix()
{
    drs::engine::EngineFacade facade;
    require(facade.getPreviewActivationPayload() != nullptr,
            "Facade completion must retain the preview payload after worker results drain.");
    require(facade.stageDraftRevision(1), "Facade EG2 matrix should stage revision 1.");
    require(facade.refreshPreviewToCurrentDraft() && settleFacade(facade),
            "Facade EG2 matrix should complete revision 1 preview.");
    const auto previewPayload = facade.getPreviewActivationPayload();
    require(previewPayload != nullptr && previewPayload->revision == 1,
            "Facade should expose its last-known-good preview payload.");
    require(facade.publishCurrentDraft() && settleFacade(facade),
            "Facade EG2 matrix should complete revision 1 publish.");
    const auto performancePayload = facade.getPerformanceActivationPayload();
    const auto snapshot = facade.getPerformanceSnapshot();
    require(performancePayload != nullptr && performancePayload->revision == 1
                && snapshot.previewActivationPayloadBytes == previewPayload->retainedPreparedBytes
                && snapshot.publishedActivationPayloadBytes == performancePayload->retainedPreparedBytes
                && snapshot.retainedActivationPayloadBytes
                    == snapshot.previewActivationPayloadBytes + snapshot.publishedActivationPayloadBytes
                && snapshot.preparedCacheResidentBytes > 0,
            "Facade metrics must distinguish and reconcile Preview and Performance payload retention.");
    facade.closeDraftPlaybackProject();
    require(facade.getPreviewActivationPayload() == nullptr
                && facade.getPerformanceActivationPayload() == nullptr,
            "Facade project close should release both retained payloads.");
}

void runProcessorHandoffMatrix()
{
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 128);
    processor.getEngineFacade().resetSessionStateToDefault();
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle()
                && processor.serviceMessageThreadWork(),
            "EG2 old-voice matrix should explicitly install the revision 0 Performance payload.");
    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;

    processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
    processor.processBlock(buffer, midi);
    auto safety = processor.getRealtimeSafetySnapshot();
    require(safety.performanceActiveVoiceCount >= 1,
            "EG2 old-voice matrix requires an active performance voice.");

    require(processor.getEngineFacade().stageDraftRevision(1),
            "Processor EG2 matrix should stage revision 1.");
    require(processor.getEngineFacade().refreshPreviewToCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            "Processor EG2 matrix should settle preview preparation.");
    require(processor.getEngineFacade().publishCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            "Processor EG2 matrix should settle publish preparation.");
    require(processor.serviceMessageThreadWork(),
            "Message owner should stage the immutable payload into a bounded slot.");
    safety = processor.getRealtimeSafetySnapshot();
    require(safety.pendingPublishedRevision == 1 && safety.pendingActivationPayloadBytes > 0,
            "Pending block-boundary activation should report retained payload bytes.");

    processor.processBlock(buffer, midi);
    safety = processor.getRealtimeSafetySnapshot();
    require(safety.activePublishedRevision == 1
                && safety.pendingPublishedRevision == 0
                && safety.activeActivationPayloadBytes > 0
                && safety.retiredActivationPayloadBytes > 0,
            "Audio callback should perform only the bounded slot exchange and queue old payload retirement.");

    processor.serviceMessageThreadWork();
    safety = processor.getRealtimeSafetySnapshot();
    require(safety.retiredActivationBacklog >= 1 && safety.retiredActivationPayloadBytes > 0,
            "Old voice lease must keep the retired payload and PCM reachable.");

    processor.queuePerformanceSurfaceNoteOff(57);
    for (int block = 0; block < 20; ++block)
        processor.processBlock(buffer, midi);
    require(processor.serviceMessageThreadWork(),
            "Message owner should reclaim the payload after the final old-voice lease releases.");
    safety = processor.getRealtimeSafetySnapshot();
    require(safety.retiredActivationBacklog == 0
                && safety.retiredActivationPayloadBytes == 0
                && safety.reclaimedActivationPayloadCount >= 1
                && safety.largeResourceReleasesOnAudioThread == 0,
            "Payload cleanup must drain off audio with reconciled retirement metrics.");
}
} // namespace

int main()
{
    try
    {
        const auto project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(project.loaded, "EG2 requires the Phase 2 reference project.");
        const auto manifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(manifest.loaded, "EG2 requires the Phase 1 reference manifest.");
        const auto stream = drs::engine::loadRuntimeStreamContainerForInstrument(manifest);
        require(stream.loaded, "EG2 requires the Phase 1 reference stream.");

        runContractLifetimeMatrix(project.project, stream);
        runFacadeRetentionMatrix();
        runProcessorHandoffMatrix();
        std::cout << "Sprint 4 Entry Gate EG2 immutable activation payload matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
