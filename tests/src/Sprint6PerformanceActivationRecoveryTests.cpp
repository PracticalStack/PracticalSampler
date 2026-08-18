#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerRenderModel.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <chrono>
#include <cmath>
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

void crossBlockBoundary(drs::plugin::Processor& processor, int blockSize = 256)
{
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForPendingPublish(drs::plugin::Processor& processor,
                           std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        const auto realtime = processor.getRealtimeSafetySnapshot();
        if (controller.activationState == drs::engine::PerformancePublishActivationState::pending
            && controller.currentRequest.identity.draftRevision == revision
            && controller.pendingActivationToken != 0
            && realtime.pendingPublishedRevision == revision
            && realtime.pendingPreparedBuildId == controller.acceptedPreparedBuildId)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto controller = processor.getPerformancePublishControllerSnapshot();
    const auto realtime = processor.getRealtimeSafetySnapshot();
    std::cerr << "Pending timeout: preparation=" << static_cast<int>(controller.preparationState)
              << ", activation=" << static_cast<int>(controller.activationState)
              << ", authorized=" << controller.activationAuthorizedCount
              << ", authorizationRejected=" << controller.activationAuthorizationRejectedCount
              << ", stagingRejected=" << controller.activationStagingRejectedCount
              << ", failure=" << controller.failureFinding.code
              << ", activeRevision=" << realtime.activePublishedRevision
              << ", pendingRevision=" << realtime.pendingPublishedRevision
              << std::endl;
    return false;
}

bool waitForActivePublish(drs::plugin::Processor& processor,
                          std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        crossBlockBoundary(processor);
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        if (controller.hasActiveRequest
            && controller.activationState == drs::engine::PerformancePublishActivationState::active
            && controller.activeRequestIdentity.draftRevision == revision)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

bool waitForBootstrapLastKnownGood(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto snapshot = processor.getRealtimeSafetySnapshot();
        if (snapshot.activePublishedRevision == 0
            && snapshot.activeActivationPayloadBytes > 0)
        {
            return true;
        }

        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    const auto snapshot = processor.getRealtimeSafetySnapshot();
    const auto controller = processor.getPerformancePublishControllerSnapshot();
    std::cerr << "Bootstrap timeout: activeBytes=" << snapshot.activeActivationPayloadBytes
              << ", activeRevision=" << snapshot.activePublishedRevision
              << ", pendingRevision=" << snapshot.pendingPublishedRevision
              << ", performanceActivations=" << snapshot.performanceActivationCount
              << ", controllerPreparation=" << static_cast<int>(controller.preparationState)
              << ", controllerActivation=" << static_cast<int>(controller.activationState)
              << ", failure=" << controller.failureFinding.code << std::endl;
    return false;
}

std::array<float, 512> renderOfflineBlock(
    drs::engine::SamplerPlaybackContext& context,
    const drs::engine::SamplerEventBlock& events)
{
    std::array<float, 256> left {};
    std::array<float, 256> right {};
    float* channels[] { left.data(), right.data() };
    const drs::engine::SamplerAudioBufferView output { channels, 2, 256 };
    const auto result = context.renderBlock(output, events.view());
    require(result.accepted, "Offline recovery render must accept its bounded block.");
    std::array<float, 512> interleaved {};
    for (std::size_t frame = 0; frame < left.size(); ++frame)
    {
        interleaved[frame * 2] = left[frame];
        interleaved[frame * 2 + 1] = right[frame];
    }
    return interleaved;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        const auto project = loadPhase2ReferenceProjectManifest();
        require(project.loaded, "Sprint 6.5 requires the authored reference project.");

        auto processorOwner = std::make_unique<drs::plugin::Processor>();
        auto& processor = *processorOwner;
        processor.prepareToPlay(48000.0, 256);
        processor.replaceAuthoringProject(project.project);
        require(waitForBootstrapLastKnownGood(processor),
                "Recovery coverage requires the bootstrap last-known-good payload to activate before Publish recovery runs.");
        const auto bootstrap = processor.getRealtimeSafetySnapshot();
        require(bootstrap.activePublishedRevision == 0
                    && bootstrap.activeActivationPayloadBytes > 0,
                "Recovery coverage requires one audible bootstrap last-known-good payload.");

        const auto firstRevision = processor.getAuthoringSession().getDocumentState().revision;
        const auto activationsBeforePublish = bootstrap.performanceActivationCount;
        require(processor.getEngineFacade().publishCurrentDraft(),
                "The first explicit Publish must enter preparation.");
        require(waitForPendingPublish(processor, firstRevision),
                "The exact prepared result must become controller-authorized before the audio boundary.");
        const auto firstPendingController = processor.getPerformancePublishControllerSnapshot();
        const auto firstPendingRealtime = processor.getRealtimeSafetySnapshot();
        require(firstPendingController.activationAuthorizedCount == 1
                    && firstPendingController.pendingPayloadBytes > 0
                    && firstPendingRealtime.activePublishedRevision == 0
                    && firstPendingRealtime.pendingPublishedRevision == firstRevision,
                "Authorization must retain bootstrap last-known-good until one block-boundary exchange.");

        require(waitForActivePublish(processor, firstRevision),
                "One audio block boundary must acknowledge the exact authorized activation.");
        const auto firstActiveController = processor.getPerformancePublishControllerSnapshot();
        const auto firstActiveRealtime = processor.getRealtimeSafetySnapshot();
        require(firstActiveController.activationCount == 1
                    && firstActiveController.activeActivationToken
                        == firstPendingController.pendingActivationToken
                    && firstActiveController.activeSnapshotBuildId
                        == firstPendingController.pendingSnapshotBuildId
                    && firstActiveController.activePreparedBuildId
                        == firstActiveRealtime.activePreparedBuildId
                    && firstActiveRealtime.activePublishedRevision == firstRevision
                    && firstActiveRealtime.pendingPublishedRevision == 0
                    && firstActiveRealtime.performanceActivationCount
                        == activationsBeforePublish + 1,
                "Success must change controller, context, and diagnostics exactly once.");

        const auto activationCountBeforeDuplicate = firstActiveRealtime.performanceActivationCount;
        const auto tokenBeforeDuplicate = firstActiveController.activeActivationToken;
        require(processor.getEngineFacade().publishCurrentDraft(),
                "An exact active Publish duplicate must remain an accepted no-op.");
        for (int block = 0; block < 4; ++block)
            crossBlockBoundary(processor);
        const auto duplicateController = processor.getPerformancePublishControllerSnapshot();
        const auto duplicateRealtime = processor.getRealtimeSafetySnapshot();
        require(duplicateController.activeActivationToken == tokenBeforeDuplicate
                    && duplicateController.activationCount == firstActiveController.activationCount
                    && duplicateRealtime.performanceActivationCount == activationCountBeforeDuplicate,
                "Repeated activation of an exact active identity must be suppressed end to end.");

        const auto activePayload = processor.getEngineFacade().getPerformanceActivationPayload();
        const auto activeModel = buildSamplerRenderModel(activePayload);
        require(activeModel.built && activeModel.model != nullptr,
                "Offline last-known-good coverage requires the active immutable render model.");
        auto referenceOwner = std::make_unique<SamplerPlaybackContext>(PlaybackActivationLane::performance);
        auto rejectedOwner = std::make_unique<SamplerPlaybackContext>(PlaybackActivationLane::performance);
        auto& reference = *referenceOwner;
        auto& rejected = *rejectedOwner;
        require(reference.prepare(48000.0) && rejected.prepare(48000.0)
                    && reference.stageActivation(activeModel.model)
                    && rejected.stageActivation(activeModel.model)
                    && reference.activatePendingForPreparation()
                    && rejected.activatePendingForPreparation(),
                "Offline contexts must install the same last-known-good model.");
        require(rejected.stageActivation(activeModel.model)
                    && rejected.cancelPendingActivation()
                    && !rejected.getSnapshot().hasPendingActivation
                    && !rejected.stageActivation({}),
                "Canceled and invalid replacements must leave active Performance untouched.");
        SamplerEventBlock noteOn;
        require(noteOn.push({ SamplerRenderEventType::noteOn, 0, 57, 0.75f }),
                "Offline recovery event must fit its bounded block.");
        const auto referenceAudio = renderOfflineBlock(reference, noteOn);
        const auto rejectedAudio = renderOfflineBlock(rejected, noteOn);
        require(referenceAudio == rejectedAudio
                    && rejected.getSnapshot().activePreparedBuildId
                        == reference.getSnapshot().activePreparedBuildId,
                "Rejected staging must change last-known-good output zero times, sample for sample.");

        processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
        crossBlockBoundary(processor);
        require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
                "Replacement coverage requires a selected authored zone.");
        auto zone = processor.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "The replacement zone must remain selected.");
        zone->gainDb -= 0.75;
        require(processor.getAuthoringSession().updateSelectedZone(
                    *zone, "Sprint 6.5 replacement activation").applied,
                "A valid edit must advance the replacement revision.");
        processor.serviceMessageThreadWork();
        const auto replacementRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(replacementRevision > firstRevision
                    && processor.getEngineFacade().publishCurrentDraft(),
                "A newer explicit revision must create a replacement activation.");
        require(waitForPendingPublish(processor, replacementRevision),
                "The replacement must reach one exact pending authorization.");
        const auto beforeReplacementBoundary = processor.getRealtimeSafetySnapshot();
        require(beforeReplacementBoundary.activePublishedRevision == firstRevision,
                "Preparation and staging must leave exact last-known-good active until the boundary.");
        require(waitForActivePublish(processor, replacementRevision),
                "The replacement must activate at one boundary.");
        auto replacementRealtime = processor.getRealtimeSafetySnapshot();
        const auto replacementController = processor.getPerformancePublishControllerSnapshot();
        require(replacementController.activationCount == firstActiveController.activationCount + 1
                    && replacementRealtime.activePublishedRevision == replacementRevision
                    && replacementRealtime.performanceActivationCount
                        == activationCountBeforeDuplicate + 1
                    && replacementRealtime.retiredActivationBacklog >= 1
                    && replacementRealtime.retiredActivationPayloadBytes > 0,
                "Replacement must activate once while retaining old voice-owned payload bytes.");

        processor.queuePerformanceSurfaceNoteOff(57);
        for (int block = 0; block < 96; ++block)
        {
            juce::AudioBuffer<float> buffer(2, 256);
            juce::MidiBuffer midi;
            buffer.clear();
            processor.processBlock(buffer, midi);
        }
        require(processor.serviceMessageThreadWork(),
                "Message service must reclaim finished retirement tokens off audio.");
        replacementRealtime = processor.getRealtimeSafetySnapshot();
        require(replacementRealtime.retiredActivationBacklog == 0
                    && replacementRealtime.retiredActivationPayloadBytes == 0
                    && replacementRealtime.reclaimedActivationPayloadCount >= 1
                    && replacementRealtime.maxActivationReclamationLatencyBlocks > 0
                    && replacementRealtime.largeResourceReleasesOnAudioThread == 0,
                "Retirement must be bounded, measurable, and reclaimed without audio-thread release.");

        std::cout << "Mini Sprint 6.5 atomic activation and recovery matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.5 activation/recovery matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
