#include "Sprint4OfflineRenderHarness.h"

#include "drs/engine/AuthoringPreviewContract.h"
#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerRenderModel.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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

std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Could not inspect " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void requireClosureSourceAudit()
{
    const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
    const auto panelHeader = readText(root / "app/src/shared/AuthoringPanel.h");
    const auto panelSource = readText(root / "app/src/shared/AuthoringPanel.cpp");
    const auto pluginShell = readText(root / "app/src/plugin/PluginEditor.cpp");
    const auto standaloneShell = readText(root / "app/src/standalone/MainComponent.cpp");
    const auto processorHeader = readText(root / "app/src/plugin/PluginProcessor.h");

    require(panelHeader.find("NotePreviewStartedCallback") == std::string::npos
                && panelHeader.find("NotePreviewEndedCallback") == std::string::npos
                && panelSource.find("onNotePreviewStarted") == std::string::npos
                && panelSource.find("onNotePreviewEnded") == std::string::npos,
            "Authoring UI must retain only the typed Preview command route.");
    require(pluginShell.find("queueAuthoringPreviewNoteOn") == std::string::npos
                && pluginShell.find("queueAuthoringPreviewNoteOff") == std::string::npos
                && standaloneShell.find("queueAuthoringPreviewNoteOn") == std::string::npos
                && standaloneShell.find("queueAuthoringPreviewNoteOff") == std::string::npos,
            "Shells must not retain duplicate untyped authoring audition callbacks.");
    require(processorHeader.find("failedAuthoringPreviewRevision") == std::string::npos
                && processorHeader.find("failedAuthoringPreviewState") == std::string::npos,
            "Processor must derive failure presentation from the controller instead of duplicate state.");
}

void requireNewestWinsReordering()
{
    using namespace drs::engine;
    AuthoringPreviewController controller({ 0, 0, 8, 2 });
    const auto old = controller.request(
        AuthoringPreviewScope::selectedZone, 1, "pad-a3-high",
        AuthoringPreviewRequestReason::authoringChanged,
        AuthoringPreviewInvalidationCategory::gain, "closure-old", 10);
    require(old.accepted && controller.launchIfEligible(10).launched,
            "Closure reordering coverage requires old in-flight work.");
    const auto newest = controller.request(
        AuthoringPreviewScope::selectedZone, 2, "pad-a3-low",
        AuthoringPreviewRequestReason::selectionChanged,
        AuthoringPreviewInvalidationCategory::selection, "closure-newest", 20);
    require(newest.accepted && newest.cancellationRequested
                && !controller.acceptPrepared(old.request.identity, 100, 30, "old", "old"),
            "A physically late obsolete result must remain rejected.");
    require(controller.launchIfEligible(20).launched
                && controller.acceptPrepared(newest.request.identity, 101, 40, "new", "new")
                && controller.markActivationPending(newest.request.identity, 45)
                && controller.markActive(newest.request.identity, 50),
            "Only the newest reordered completion may activate.");
}

void crossBlock(drs::plugin::Processor& processor, int blockSize = 256)
{
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForPreviewReady(drs::plugin::Processor& processor,
                         std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        if (processor.getAuthoringPreviewControllerSnapshot().preparationState
            == drs::engine::AuthoringPreviewPreparationState::ready)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

drs::tests::OfflineRenderArtifact renderPerformanceIdentity(
    const drs::engine::PlaybackActivationPayloadPtr& payload,
    const std::string& scenario)
{
    const auto model = drs::engine::buildSamplerRenderModel(payload);
    require(model.built && model.model != nullptr,
            "Performance stability evidence requires a valid immutable render model.");
    drs::tests::OfflineRenderRequest request;
    request.scenarioId = scenario;
    request.model = model.model;
    request.sampleRate = 48000.0;
    request.frameCount = 4096;
    request.partitionSize = 127;
    request.events = {
        { 0, drs::engine::SamplerRenderEventType::noteOn, 57, 0.78f },
        { 2048, drs::engine::SamplerRenderEventType::noteOff, 57, 0.0f }
    };
    return drs::tests::renderOffline(request);
}

drs::engine::RuntimeProjectModel makeUnloadedProject()
{
    drs::engine::RuntimeProjectModel unloaded;
    unloaded.schemaName = "drs.project";
    unloaded.schemaVersion = 2;
    unloaded.displayName = "No Project Loaded";
    unloaded.authoring.schemaName = "drs.authoring";
    unloaded.authoring.schemaVersion = 1;
    return unloaded;
}

void runIntegratedClosureSoak(const drs::engine::RuntimeProjectModel& sourceProject)
{
    using Budgets = drs::engine::AuthoringPreviewIntegrationBudgets;
    auto project = sourceProject;
    project.authoring.selectedZoneId = "pad-a3-high";

    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    processor.replaceAuthoringProject(project);
    require(waitForPreviewReady(processor), "Initial Preview must prepare before closure churn.");
    crossBlock(processor);
    std::cerr << "S5.8 stage: initial Preview active" << std::endl;

    require(processor.getEngineFacade().publishCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::seconds(10)),
            "Closure soak requires an initial published Performance model.");
    processor.serviceMessageThreadWork();
    crossBlock(processor);
    const auto performancePayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(performancePayload != nullptr, "Closure soak requires retained Performance identity.");
    const auto performanceRevision = performancePayload->revision;
    const auto performanceBuildId = performancePayload->preparedBuildId;
    const auto performanceDigest = performancePayload->preparedContentDigest;
    const auto baseline = renderPerformanceIdentity(performancePayload, "s5-before-preview-churn");
    std::cerr << "S5.8 stage: Performance baseline captured" << std::endl;

    std::atomic<bool> start { false };
    std::atomic<bool> messageComplete { false };
    std::atomic<bool> audioComplete { false };
    std::atomic<bool> coherent { true };
    std::atomic<std::size_t> statusPollCount { 0 };
    std::atomic<std::size_t> maximumRetirementBacklog { 0 };
    std::atomic<std::uint64_t> maximumRetainedBytes { 0 };

    processor.queuePerformanceSurfaceNoteOn(57, 0.78f);
    std::thread audioThread([&]
    {
        // Use the largest supported callback block so an involuntary desktop/CI
        // scheduler preemption is not misreported as sampler work exceeding its
        // realtime deadline. The dedicated realtime matrix covers every supported
        // block-size/sample-rate profile.
        juce::AudioBuffer<float> buffer(2, 1024);
        juce::MidiBuffer midi;
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();
        auto blocks = std::size_t { 0 };
        while ((!messageComplete.load(std::memory_order_acquire) || blocks < 150)
               && coherent.load(std::memory_order_acquire))
        {
            buffer.clear();
            processor.processBlock(buffer, midi);
            midi.clear();
            ++blocks;
            if ((blocks % 32) == 0)
                std::this_thread::yield();
        }
        audioComplete.store(true, std::memory_order_release);
    });

    std::thread statusReader([&]
    {
        while (!start.load(std::memory_order_acquire))
            std::this_thread::sleep_for(std::chrono::microseconds(250));
        while ((!messageComplete.load(std::memory_order_acquire)
                || !audioComplete.load(std::memory_order_acquire))
               && coherent.load(std::memory_order_acquire))
        {
            const auto status = processor.getAuthoringPreviewStatusSnapshot();
            const auto diagnostics = processor.getRealtimeSafetySnapshot();
            if (!status.available
                || (status.activePreparedBuildId != 0
                    && (status.activeSnapshotDigest.empty() || status.activePreparedDigest.empty()))
                || (status.usingLastKnownGood && status.activePreparedBuildId == 0))
            {
                coherent.store(false, std::memory_order_release);
                break;
            }
            auto backlog = maximumRetirementBacklog.load(std::memory_order_relaxed);
            while (backlog < diagnostics.retiredActivationBacklog
                   && !maximumRetirementBacklog.compare_exchange_weak(
                       backlog, diagnostics.retiredActivationBacklog, std::memory_order_relaxed))
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
    for (auto edit = 0; edit < 8; ++edit)
    {
        const auto* selectedZoneId = ((edit / 4) & 1) == 0
            ? "pad-a3-high" : "pad-a3-low";
        processor.getAuthoringSession().selectZone(selectedZoneId);
        auto zone = processor.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "Mixed closure churn must retain a selected zone.");
        zone->gainDb = -3.0 + static_cast<double>(edit % 7) * 0.25;
        zone->pan = -0.5 + static_cast<double>(edit % 9) * 0.125;
        zone->rootKey = std::clamp(zone->rootKey + ((edit % 3) - 1), 0, 127);
        require(processor.getAuthoringSession().updateSelectedZone(
                    *zone, edit % 2 == 0 ? "Closure gain edit" : "Closure pan edit").applied,
                "Mixed closure edit must advance authored state.");

        drs::engine::AuthoringPreviewCommand audition;
        audition.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
        audition.source = static_cast<drs::engine::AuthoringPreviewAuditionSource>(edit % 4);
        audition.midiNote = zone->rootKey;
        audition.velocity = 0.65f;
        audition.selectedZoneId = zone->id;
        require(processor.submitAuthoringPreviewCommand(audition),
                "Every mixed closure audition command must be accepted.");
        if ((edit % 3) == 2)
        {
            drs::engine::AuthoringPreviewCommand stop;
            stop.type = drs::engine::AuthoringPreviewCommandType::stopAll;
            stop.source = audition.source;
            require(processor.submitAuthoringPreviewCommand(stop),
                    "Preview stop must remain lane-local during closure churn.");
        }
        processor.serviceMessageThreadWork();
    }
    std::cerr << "S5.8 stage: edit churn submitted" << std::endl;
    auto newestBecameActive = false;
    const auto activationDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < activationDeadline)
    {
        processor.serviceMessageThreadWork();
        const auto current = processor.getAuthoringPreviewStatusSnapshot();
        if (current.presentationState
                == drs::engine::AuthoringPreviewPresentationState::active
            && current.activeRevision == current.draftRevision)
        {
            newestBecameActive = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    messageComplete.store(true, std::memory_order_release);
    audioThread.join();
    std::cerr << "S5.8 stage: audio joined" << std::endl;
    statusReader.join();
    std::cerr << "S5.8 stage: status reader joined" << std::endl;
    require(coherent.load(std::memory_order_acquire) && statusPollCount.load() > 0,
            "Concurrent immutable status polling must remain coherent through closure churn.");

    require(newestBecameActive, "Newest closure request must settle and activate within ten seconds.");
    std::cerr << "S5.8 stage: newest Preview active" << std::endl;
    drs::engine::AuthoringPreviewCommand stop;
    stop.type = drs::engine::AuthoringPreviewCommandType::stopAll;
    processor.submitAuthoringPreviewCommand(stop);
    processor.queuePerformanceSurfaceNoteOff(57);
    for (auto block = 0; block < 64; ++block)
        crossBlock(processor);
    std::cerr << "S5.8 stage: retirement tail complete" << std::endl;

    const auto status = processor.getAuthoringPreviewStatusSnapshot();
    const auto controller = processor.getAuthoringPreviewControllerSnapshot();
    const auto worker = processor.getEngineFacade().getPreparedPlaybackWorkerStatus();
    const auto diagnostics = processor.getRealtimeSafetySnapshot();
    require(status.presentationState == drs::engine::AuthoringPreviewPresentationState::active
                && status.activeRevision == status.draftRevision
                && controller.activationCount >= 2,
            "Only the newest successful closure request may become active.");
    require(controller.configuredMaximumLaunchDelayMicros
                    <= Budgets::maximumCoalescingDelayMicros
                && controller.maximumPendingDepth <= Budgets::maximumControllerPendingDepth
                && worker.maxPendingWorkCount <= Budgets::maximumWorkerPendingWorkCount
                && worker.inFlightWorkCount <= Budgets::maximumWorkerInFlightWorkCount,
            "Controller and physical preparation work must remain within closure bounds.");
    std::cerr << "S5.8 metrics: max request-to-audible="
              << status.maxRequestToAudibleMicros << " us" << std::endl;
    require(status.maxRequestToAudibleMicros <= Budgets::maximumRequestToAudibleMicros,
            "Measured request-to-audible latency exceeded the Sprint 5 support budget.");
    require(maximumRetainedBytes.load() <= Budgets::maximumRetainedActivationBytes
                && maximumRetirementBacklog.load() <= Budgets::maximumRetirementBacklog
                && diagnostics.retiredActivationBacklog == 0
                && diagnostics.retiredActivationPayloadBytes == 0,
            "Activation retention or retirement exceeded the Sprint 5 bounded-lifetime budget.");
    std::cerr << "S5.8 realtime metrics: performanceEventsDropped="
              << diagnostics.performanceDroppedEventCount
              << " previewEventsDropped=" << diagnostics.authoringPreviewDroppedEventCount
              << " performanceNotesDropped=" << diagnostics.performanceDroppedNoteCount
              << " previewNotesDropped=" << diagnostics.authoringPreviewDroppedNoteCount
              << " overBudget=" << diagnostics.overBudgetCallbackCount
              << " violations=" << diagnostics.getAudioThreadViolationCount()
              << std::endl;
    require(diagnostics.performanceDroppedEventCount
                    + diagnostics.authoringPreviewDroppedEventCount
                    + diagnostics.performanceDroppedNoteCount
                    + diagnostics.authoringPreviewDroppedNoteCount
                    <= Budgets::maximumQueueDropCount
                && diagnostics.overBudgetCallbackCount <= Budgets::maximumCallbackOverrunCount
                && diagnostics.getAudioThreadViolationCount() == 0,
            "Closure churn must remain drop-free and realtime-clean.");
    std::cerr << "S5.8 metrics: worker cancellation=" << worker.cancellationCount
              << " superseded=" << worker.supersededCount
              << " retired=" << diagnostics.retiredActivationCount
              << " reclaimed=" << diagnostics.reclaimedActivationPayloadCount
              << " performancePeak=" << diagnostics.performancePeakActiveVoiceCount
              << " previewPeak=" << diagnostics.authoringPreviewPeakActiveVoiceCount
              << std::endl;
    require(diagnostics.retiredActivationCount > 0
                && diagnostics.reclaimedActivationPayloadCount > 0
                && diagnostics.performancePeakActiveVoiceCount > 0
                && diagnostics.authoringPreviewPeakActiveVoiceCount > 0,
            "Closure soak must exercise both lanes and off-audio retirement.");

    const auto afterChurnPayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(afterChurnPayload == performancePayload
                && afterChurnPayload->revision == performanceRevision
                && afterChurnPayload->preparedBuildId == performanceBuildId
                && afterChurnPayload->preparedContentDigest == performanceDigest,
            "Unsaved Preview edits must not mutate active Performance identity.");
    const auto afterChurn = renderPerformanceIdentity(afterChurnPayload,
                                                      "s5-after-preview-churn");
    require(drs::tests::compareOfflineArtifacts(baseline, afterChurn).equivalent,
            "Unsaved Preview edits must leave Performance output byte-stable.");

    processor.closeAuthoringProject(makeUnloadedProject());
    crossBlock(processor);
    std::cerr << "S5.8 stage: project closed" << std::endl;
    const auto closedStatus = processor.getAuthoringPreviewStatusSnapshot();
    const auto closedDiagnostics = processor.getRealtimeSafetySnapshot();
    const auto closedPerformance = processor.getEngineFacade().getPerformanceActivationPayload();
    std::cerr << "S5.8 close metrics: previewBuild=" << closedStatus.activePreparedBuildId
              << " previewVoices=" << closedDiagnostics.authoringPreviewActiveVoiceCount
              << " performancePayload=" << (closedPerformance != nullptr)
              << " performanceBuild=" << (closedPerformance != nullptr
                    ? closedPerformance->preparedBuildId : 0)
              << std::endl;
    require(closedStatus.activePreparedBuildId == 0
                && closedDiagnostics.authoringPreviewActiveVoiceCount == 0
                && closedDiagnostics.activePublishedRevision == performanceRevision,
            "Project Close must clear Preview without changing Performance.");
    processor.replaceAuthoringProject(project);
    require(waitForPreviewReady(processor), "Project reopen must prepare a fresh Preview.");
    crossBlock(processor);
    std::cerr << "S5.8 stage: project reopened" << std::endl;
    require(processor.getAuthoringPreviewStatusSnapshot().activePreparedBuildId != 0
                && processor.getRealtimeSafetySnapshot().activePublishedRevision == performanceRevision,
            "Project reopen must recover Preview while preserving published Performance.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        requireClosureSourceAudit();
        requireNewestWinsReordering();
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 5.8 requires the authored reference project.");
        runIntegratedClosureSoak(loaded.project);
        std::cout << "Mini Sprint 5.8 integration hardening and budget matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.8 integration hardening failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
