#include "drs/engine/PerformancePublishContract.h"
#include "drs/engine/PerformancePublishController.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::PerformancePublishResult eligibleResult(
    const drs::engine::PerformancePublishRequestIdentity& identity,
    std::uint64_t buildId)
{
    drs::engine::PerformancePublishResult result;
    result.identity = identity;
    result.completeProject = true;
    result.activationEligible = true;
    result.preparedBuildId = buildId;
    result.preparedContentDigest = "prepared:" + std::to_string(buildId);
    result.routeDigest = "routes:" + std::to_string(buildId);
    result.sourceProvenanceDigest = "sources:" + std::to_string(buildId);
    result.preparedMacroSchemaDigest = identity.macroSchemaDigest;
    return result;
}

drs::engine::PerformancePublishActivationPayload activationPayload(
    const drs::engine::PerformancePublishResult& result,
    std::uint64_t token)
{
    using namespace drs::engine;
    PerformancePublishActivationPayload payload;
    payload.activationToken = token;
    payload.requestIdentity = result.identity;
    payload.revision = result.identity.draftRevision;
    payload.snapshotBuildId = 10000 + result.preparedBuildId;
    payload.preparedBuildId = result.preparedBuildId;
    payload.snapshotContentDigest = result.identity.authoredContentDigest;
    payload.preparedContentDigest = result.preparedContentDigest;
    payload.routeDigest = result.routeDigest;
    payload.sourceProvenanceDigest = result.sourceProvenanceDigest;
    payload.macroSchemaDigest = result.preparedMacroSchemaDigest;
    payload.retainedPreparedBytes = 4096;
    auto macros = std::make_shared<ImmutablePublishedMacroBindingTable>();
    macros->revision = payload.revision;
    macros->macroSchemaDigest = payload.macroSchemaDigest;
    macros->callbackView.revision = payload.revision;
    payload.macroBindings = std::move(macros);
    auto playback = std::make_shared<PlaybackActivationPayload>();
    playback->lane = PlaybackActivationLane::performance;
    playback->revision = payload.revision;
    playback->snapshotBuildId = payload.snapshotBuildId;
    playback->preparedBuildId = payload.preparedBuildId;
    playback->lifecycleState = PlaybackSnapshotLifecycleState::active;
    playback->activationEligible = true;
    playback->snapshotContentDigest = payload.snapshotContentDigest;
    playback->preparedContentDigest = payload.preparedContentDigest;
    playback->routeDigest = payload.routeDigest;
    playback->sourceProvenanceDigest = payload.sourceProvenanceDigest;
    playback->macroSchemaDigest = payload.macroSchemaDigest;
    playback->retainedPreparedBytes = payload.retainedPreparedBytes;
    playback->snapshot = std::make_shared<ImmutablePlaybackSnapshot>();
    playback->prepared = std::make_shared<ImmutablePreparedPlayback>();
    payload.playbackPayload = std::move(playback);
    return payload;
}

void requireFailureRepairAndReordering()
{
    using namespace drs::engine;
    PerformancePublishController controller({ 8 });
    const auto first = controller.request(11, 1, "authored:1", "macros:1", 10);
    const auto firstResult = eligibleResult(first.request.identity, 101);
    const auto firstPayload = activationPayload(firstResult, 1001);
    require(first.accepted && controller.markPreparing(first.request.identity, 20)
                && controller.acceptPrepared(firstResult, 30)
                && controller.authorizeActivation(firstPayload, 40)
                && controller.acknowledgeActivation(firstPayload, 50),
            "Closure setup must establish one immutable last-known-good publication.");

    const auto failed = controller.request(11, 2, "authored:2", "macros:2", 60);
    require(failed.accepted && controller.fail(
                failed.request.identity,
                { PerformancePublishFindingSeverity::error,
                  "closure-invalid-route", "authoring.routes", "Repair the route." }),
            "Closure coverage must accept a typed failed publication.");
    auto snapshot = controller.getSnapshot();
    require(snapshot.hasFailedRequest && snapshot.hasActiveRequest
                && snapshot.activeRequestIdentity == first.request.identity
                && controller.getActiveActivationPayload() != nullptr
                && controller.getActiveActivationPayload()->activationToken == 1001,
            "Failure must preserve the exact controller-owned last-known-good payload.");

    const auto obsolete = controller.request(11, 3, "authored:3", "macros:3", 70);
    require(obsolete.accepted && controller.markPreparing(obsolete.request.identity, 80),
            "Reorder coverage requires an older request in flight.");
    const auto repaired = controller.request(11, 4, "authored:4", "macros:4", 90);
    const auto obsoleteResult = eligibleResult(obsolete.request.identity, 102);
    const auto repairedResult = eligibleResult(repaired.request.identity, 103);
    const auto repairedPayload = activationPayload(repairedResult, 1002);
    require(repaired.accepted && repaired.supersededPrevious
                && !controller.acceptPrepared(obsoleteResult, 100)
                && controller.markPreparing(repaired.request.identity, 110)
                && controller.acceptPrepared(repairedResult, 120)
                && controller.authorizeActivation(repairedPayload, 130)
                && controller.acknowledgeActivation(repairedPayload, 140),
            "A repaired newest request must reject reordered obsolete work and activate exactly once.");
    snapshot = controller.getSnapshot();
    require(snapshot.activeRequestIdentity == repaired.request.identity
                && snapshot.activationCount == 2
                && snapshot.rejectedCount > 0
                && snapshot.supersededCount > 0
                && snapshot.maximumPendingDepth <= 1
                && snapshot.retainedCompletionRecordCount <= 8,
            "Failure/repair/reorder state must remain bounded and newest-wins.");
}

void processBoundary(drs::plugin::Processor& processor, int blockSize = 1024)
{
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForActive(drs::plugin::Processor& processor,
                   std::size_t revision,
                   std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto presentation = processor.getPerformancePublishPresentationSnapshot();
        if (presentation != nullptr
            && presentation->state == drs::engine::PerformancePublishPresentationState::active
            && presentation->activePublishedRevision == revision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

bool waitForActiveAcrossBoundaries(drs::plugin::Processor& processor,
                                   std::size_t revision,
                                   std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processBoundary(processor);
        const auto presentation = processor.getPerformancePublishPresentationSnapshot();
        if (presentation != nullptr
            && presentation->state == drs::engine::PerformancePublishPresentationState::active
            && presentation->activePublishedRevision == revision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

void runIntegratedPublishSoak(const drs::engine::RuntimeProjectModel& sourceProject)
{
    using Budgets = drs::engine::PerformancePublishIntegrationBudgets;
    using namespace drs::engine;

    auto project = sourceProject;
    project.authoring.selectedZoneId = "pad-a3-high";
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 1024);
    processor.replaceAuthoringProject(project);
    processor.serviceMessageThreadWork();

    const auto initialRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(processor.submitPerformancePublishCommand(
                {}, PerformancePublishCommandSource::externalApi),
            "Closure soak requires an explicit initial Publish.");
    require(waitForActiveAcrossBoundaries(processor, initialRevision),
            "Initial explicit Publish must become active within the support timeout.");
    const auto initialPayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(initialPayload != nullptr && initialPayload->revision == initialRevision,
            "Closure soak requires the exact initial immutable Performance payload.");

    processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
    processBoundary(processor);
    const auto heldGeneration = processor.getRealtimeSafetySnapshot().performanceActiveGeneration;

    std::atomic<bool> start { false };
    std::atomic<bool> stop { false };
    std::atomic<bool> audioComplete { false };
    std::atomic<bool> coherent { true };
    std::atomic<std::size_t> statusPollCount { 0 };
    std::atomic<std::size_t> maximumRetirementBacklog { 0 };
    std::atomic<std::uint64_t> maximumRetainedBytes { 0 };

    std::thread audioThread([&]
    {
        juce::AudioBuffer<float> buffer(2, 1024);
        juce::MidiBuffer midi;
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();
        auto block = std::size_t { 0 };
        while (!stop.load(std::memory_order_acquire) || block < 192)
        {
            buffer.clear();
            midi.clear();
            if ((block % 48) == 0)
                midi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(96)), 0);
            if ((block % 48) == 12)
                midi.addEvent(juce::MidiMessage::noteOff(1, 60), 32);
            processor.processBlock(buffer, midi);
            ++block;
            // Pace the 48 kHz / 1024-frame synthetic callback near its real 21.3 ms period.
            // With routed voices genuinely audible, an unthrottled Debug render loop can
            // monopolize a Windows core and measure scheduler starvation as message work.
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        audioComplete.store(true, std::memory_order_release);
    });

    std::thread statusThread([&]
    {
        auto lastSequence = std::uint64_t { 0 };
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();
        while ((!stop.load(std::memory_order_acquire)
                || !audioComplete.load(std::memory_order_acquire))
               && coherent.load(std::memory_order_acquire))
        {
            const auto presentation = processor.getPerformancePublishPresentationSnapshot();
            const auto diagnostics = processor.getRealtimeSafetySnapshot();
            if (presentation == nullptr
                || presentation->publicationSequence < lastSequence
                || (presentation->hasActivePublished
                    && (presentation->activePublishedDigest.empty()
                        || presentation->lastKnownGoodRevision
                            != presentation->activePublishedRevision
                        || presentation->lastKnownGoodDigest
                            != presentation->activePublishedDigest)))
            {
                coherent.store(false, std::memory_order_release);
                break;
            }
            lastSequence = presentation->publicationSequence;
            auto backlog = maximumRetirementBacklog.load(std::memory_order_relaxed);
            while (backlog < diagnostics.retiredActivationBacklog
                   && !maximumRetirementBacklog.compare_exchange_weak(
                       backlog, diagnostics.retiredActivationBacklog,
                       std::memory_order_relaxed))
            {
            }
            const auto bytes = diagnostics.activeActivationPayloadBytes
                + diagnostics.pendingActivationPayloadBytes
                + diagnostics.retiredActivationPayloadBytes;
            auto retained = maximumRetainedBytes.load(std::memory_order_relaxed);
            while (retained < bytes
                   && !maximumRetainedBytes.compare_exchange_weak(
                       retained, bytes, std::memory_order_relaxed))
            {
            }
            statusPollCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(250));
        }
    });

    start.store(true, std::memory_order_release);

    // Unsaved Draft/Preview churn must never mutate Performance implicitly.
    for (auto edit = 0; edit < 4; ++edit)
    {
        require(processor.getAuthoringSession().selectZone(
                    edit % 2 == 0 ? "pad-a3-high" : "pad-a3-low").applied,
                "Preview-only closure churn requires a valid selected zone.");
        auto zone = processor.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "Preview-only closure churn lost its selected zone.");
        zone->gainDb -= 0.1 + 0.05 * edit;
        zone->pan = -0.3 + 0.2 * edit;
        require(processor.getAuthoringSession().updateSelectedZone(
                    *zone, "Sprint 6.9 Preview-only edit").applied,
                "Preview-only authored edits must advance Draft.");
        AuthoringPreviewCommand preview;
        preview.type = AuthoringPreviewCommandType::auditionSelectedZone;
        preview.source = AuthoringPreviewAuditionSource::authoringKeyboard;
        preview.midiNote = zone->rootKey;
        preview.velocity = 0.65f;
        preview.selectedZoneId = zone->id;
        require(processor.submitAuthoringPreviewCommand(preview),
                "Preview audition must remain accepted during Performance rendering.");
        processor.setMacroValueFromShell("tone", 0.35 + 0.1 * edit);
        processor.serviceMessageThreadWork();
    }
    require(processor.getEngineFacade().getPerformanceActivationPayload() == initialPayload
                && processor.getRealtimeSafetySnapshot().activePublishedRevision == initialRevision,
            "Draft, Preview, host automation, and authoring notes must not publish implicitly.");

    // Repeated explicit Publish requests exercise coalescing, supersession, and activation churn.
    for (auto edit = 0; edit < 8; ++edit)
    {
        require(processor.getAuthoringSession().selectZone(
                    edit % 2 == 0 ? "pad-a3-low" : "pad-a3-high").applied,
                "Publish churn requires a valid selected zone.");
        auto zone = processor.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "Publish churn lost its selected zone.");
        zone->gainDb = -2.5 + 0.15 * edit;
        zone->pan = -0.4 + 0.1 * edit;
        require(processor.getAuthoringSession().updateSelectedZone(
                    *zone, "Sprint 6.9 explicit Publish edit").applied,
                "Every Publish churn edit must advance Draft.");
        processor.serviceMessageThreadWork();
        require(processor.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::authoringWorkspace),
                "Every distinct explicit Publish request must be accepted.");
        processor.submitPerformancePublishCommand(
            {}, PerformancePublishCommandSource::statusPanel);
        processor.setMacroValueFromShell("motion", 0.2 + 0.07 * edit);
        processor.serviceMessageThreadWork();
    }

    const auto newestRevision = processor.getAuthoringSession().getDocumentState().revision;
    require(waitForActive(processor, newestRevision),
            "The newest explicit Publish must become active within the support timeout.");
    const auto activePresentation = processor.getPerformancePublishPresentationSnapshot();
    require(activePresentation != nullptr && !activePresentation->dirty
                && activePresentation->activePublishedRevision == newestRevision
                && activePresentation->activeMacroSchemaDigest
                    == processor.getPerformancePublishControllerSnapshot().activeMacroSchemaDigest,
            "Newest active revision, macro schema, and dirty truth must be coherent.");
    const auto duringHeldNote = processor.getRealtimeSafetySnapshot();
    require(duringHeldNote.performanceActiveGeneration != heldGeneration
                && duringHeldNote.performanceRetiredGenerationVoiceCount > 0,
            "A held note must retain its old generation across explicit activation churn.");

    processor.queuePerformanceSurfaceNoteOff(57);
    AuthoringPreviewCommand stopPreview;
    stopPreview.type = AuthoringPreviewCommandType::stopAll;
    stopPreview.source = AuthoringPreviewAuditionSource::authoringKeyboard;
    processor.submitAuthoringPreviewCommand(stopPreview);
    for (auto block = 0; block < 128; ++block)
    {
        processor.serviceMessageThreadWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stop.store(true, std::memory_order_release);
    audioThread.join();
    statusThread.join();
    for (auto block = 0; block < 96; ++block)
        processBoundary(processor);

    require(coherent.load(std::memory_order_acquire) && statusPollCount.load() > 0,
            "Immutable Publish polling must remain coherent during mixed closure load.");
    const auto controller = processor.getPerformancePublishControllerSnapshot();
    const auto worker = processor.getEngineFacade().getPreparedPlaybackWorkerStatus();
    const auto diagnostics = processor.getRealtimeSafetySnapshot();
    const auto commands = processor.getPerformancePublishCommandSnapshot();
    const auto totalDrops = diagnostics.performanceDroppedEventCount
        + diagnostics.authoringPreviewDroppedEventCount
        + diagnostics.performanceDroppedNoteCount
        + diagnostics.authoringPreviewDroppedNoteCount;
    std::cerr << "S6.9 budget diagnostics: requestToActive="
              << controller.maxRequestToActiveMicros
              << ", controllerDepth=" << controller.maximumPendingDepth
              << ", workerDepth=" << worker.maxPendingWorkCount
              << ", inFlight=" << worker.inFlightWorkCount
              << ", completedDepth=" << worker.maxCompletedResultCount
              << ", messageService=" << worker.maxMessageThreadServiceMicros
              << ", messageViolations=" << worker.messageThreadServiceBudgetViolationCount
              << std::endl;
    require(controller.maxRequestToActiveMicros <= Budgets::maximumRequestToActiveMicros
                && controller.maximumPendingDepth <= Budgets::maximumControllerPendingDepth
                && worker.maxPendingWorkCount <= Budgets::maximumWorkerPendingWorkCount
                && worker.inFlightWorkCount <= Budgets::maximumWorkerInFlightWorkCount
                && worker.maxCompletedResultCount <= Budgets::maximumCompletedResultCount
                && worker.maxMessageThreadServiceMicros
                    <= Budgets::maximumMessageThreadServiceMicros
                && worker.messageThreadServiceBudgetViolationCount == 0,
            "Command latency, controller, worker, and message-service budgets must remain bounded.");
    require(maximumRetainedBytes.load() <= Budgets::maximumRetainedActivationBytes
                && maximumRetirementBacklog.load() <= Budgets::maximumRetirementBacklog
                && diagnostics.retiredActivationBacklog == 0
                && diagnostics.retiredActivationPayloadBytes == 0
                && diagnostics.performanceRetiredGenerationVoiceCount == 0,
            "Activation memory, old-generation voices, and retirement must drain within budget.");
    require(diagnostics.performancePeakActiveVoiceCount <= Budgets::maximumPerformanceVoiceCount
                && diagnostics.authoringPreviewPeakActiveVoiceCount
                    <= Budgets::maximumPreviewVoiceCount
                && totalDrops <= Budgets::maximumQueueDropCount
                && diagnostics.overBudgetCallbackCount
                    <= Budgets::maximumCallbackOverrunCount
                && diagnostics.getAudioThreadViolationCount()
                    <= Budgets::maximumAudioThreadViolationCount,
            "Voice, queue, callback, and realtime budgets must remain within support limits.");
    require(commands.acceptedCommandCount >= 9
                && commands.executionAcceptedCount >= 9
                && controller.activationCount >= 2
                && worker.performanceDispatchCount > 0
                && worker.previewDispatchCount > 0,
            "Closure load must exercise typed commands, both worker lanes, and repeated activation.");

    std::cout << "S6.9 metrics: requestToActive=" << controller.maxRequestToActiveMicros
              << "us, workerDepth=" << worker.maxPendingWorkCount
              << ", retainedBytes=" << maximumRetainedBytes.load()
              << ", retirementBacklog=" << maximumRetirementBacklog.load()
              << ", performance/preview peaks=" << diagnostics.performancePeakActiveVoiceCount
              << "/" << diagnostics.authoringPreviewPeakActiveVoiceCount
              << ", drops=" << totalDrops
              << ", overruns=" << diagnostics.overBudgetCallbackCount
              << ", violations=" << diagnostics.getAudioThreadViolationCount()
              << std::endl;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        requireFailureRepairAndReordering();
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 6.9 requires the authored reference project.");
        runIntegratedPublishSoak(loaded.project);
        std::cout << "Mini Sprint 6.9 integration hardening and closure matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.9 integration hardening failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
