#include "plugin/PluginProcessor.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>

namespace
{
bool snapshotIsCoherent(const drs::plugin::ProcessorRealtimeSafetySnapshot& snapshot,
                        std::uint64_t previousSequence,
                        std::size_t previousBlockCount) noexcept
{
    const auto voiceCount = snapshot.performanceActiveVoiceCount
        + snapshot.authoringPreviewActiveVoiceCount;
    return snapshot.available
        && (snapshot.publicationSequence & 1u) == 0
        && snapshot.publicationSequence >= previousSequence
        && snapshot.processBlockCount >= previousBlockCount
        && snapshot.performanceContextIdentity != 0
        && snapshot.authoringPreviewContextIdentity != 0
        && snapshot.performanceContextIdentity != snapshot.authoringPreviewContextIdentity
        && voiceCount <= snapshot.activeVoiceCapacityLimit
        && snapshot.activeVoiceCapacity >= voiceCount
        && snapshot.performancePeakActiveVoiceCount <= snapshot.activeVoiceCapacityLimit
        && snapshot.authoringPreviewPeakActiveVoiceCount <= snapshot.activeVoiceCapacityLimit
        && snapshot.maxPerformanceRenderMicros >= snapshot.lastPerformanceRenderMicros
        && snapshot.maxAuthoringPreviewRenderMicros >= snapshot.lastAuthoringPreviewRenderMicros;
}
} // namespace

int main()
{
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 64);

    const auto project = drs::engine::loadPhase2ReferenceProjectManifest();
    if (!project.loaded)
    {
        std::cerr << "Sprint 4 concurrency soak could not load the reference project." << std::endl;
        return 1;
    }

    processor.replaceAuthoringProject(project.project);
    if (!processor.getAuthoringSession().selectZone("pad-a3-high").applied
        || !processor.serviceMessageThreadWork()
        || !processor.getEngineFacade().refreshPreviewToCurrentDraft()
        || !processor.getEngineFacade().waitForPreparedPlaybackIdle()
        || !processor.getEngineFacade().publishCurrentDraft()
        || !processor.getEngineFacade().waitForPreparedPlaybackIdle())
    {
        std::cerr << "Sprint 4 concurrency soak could not prime both playback contexts." << std::endl;
        return 1;
    }
    // Waiting may have already drained each publication completion. Poll to
    // install any completion that remains, but do not mistake a no-op poll for
    // a failed activation; the post-soak snapshot asserts both contexts live.
    processor.serviceMessageThreadWork();

    juce::AudioBuffer<float> primingBuffer(2, 64);
    juce::MidiBuffer primingMidi;
    auto primingBlockCount = 0;
    auto initialPreviewActivated = false;
    const auto primingDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < primingDeadline)
    {
        processor.serviceMessageThreadWork();
        primingBuffer.clear();
        processor.processBlock(primingBuffer, primingMidi);
        ++primingBlockCount;
        processor.serviceMessageThreadWork();
        if (processor.getRealtimeSafetySnapshot().authoringPreviewActivationCount >= 1)
        {
            initialPreviewActivated = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    constexpr int audioBlockCount = 5000;
    constexpr int messageIterationCount = 480;
    std::atomic<bool> start { false };
    std::atomic<bool> audioComplete { false };
    std::atomic<bool> messageComplete { false };
    std::atomic<bool> failed { false };
    std::atomic<std::size_t> uiPollCount { 0 };

    std::thread audioThread([&]
    {
        juce::AudioBuffer<float> buffer(2, 64);
        juce::MidiBuffer midi;
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();

        for (int block = 0; block < audioBlockCount && !failed.load(std::memory_order_acquire); ++block)
        {
            buffer.clear();
            processor.processBlock(buffer, midi);
            if ((block % 32) == 0)
                std::this_thread::yield();
        }
        audioComplete.store(true, std::memory_order_release);
    });

    std::thread messageThread([&]
    {
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();

        for (int iteration = 0;
             iteration < messageIterationCount && !failed.load(std::memory_order_acquire);
             ++iteration)
        {
            auto previewRefreshRequested = false;
            processor.getEngineFacade().setMacroValue(
                "tone", static_cast<double>(iteration % 101) / 100.0);
            processor.getEngineFacade().setMacroValue(
                "motion", static_cast<double>((iteration * 11) % 101) / 100.0);

            if ((iteration % 8) == 0)
            {
                const auto note = 60 + ((iteration / 8) % 12);
                processor.queuePerformanceSurfaceNoteOn(note, 0.78f);
                processor.queueAuthoringPreviewNoteOn(note, 0.68f);
            }
            else if ((iteration % 8) == 4)
            {
                const auto note = 60 + ((iteration / 8) % 12);
                processor.queuePerformanceSurfaceNoteOff(note);
                processor.queueAuthoringPreviewNoteOff(note);
            }

            if ((iteration % 40) == 0)
            {
                const auto* zoneId = ((iteration / 40) & 1) == 0
                    ? "pad-a3-low"
                    : "pad-a3-high";
                if (!processor.getAuthoringSession().selectZone(zoneId).applied)
                    failed.store(true, std::memory_order_release);
                else
                {
                    auto zone = processor.getAuthoringSession().getSelectedZone();
                    if (!zone.has_value())
                        failed.store(true, std::memory_order_release);
                    else
                    {
                        zone->pan = ((iteration / 40) & 1) == 0 ? -0.25 : 0.25;
                        if (!processor.getAuthoringSession().updateSelectedZone(
                                *zone, "Concurrency soak Preview edit").applied)
                            failed.store(true, std::memory_order_release);
                        else
                        {
                            drs::engine::AuthoringPreviewCommand audition;
                            audition.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
                            audition.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
                            audition.midiNote = zone->rootKey;
                            audition.velocity = 0.68f;
                            audition.selectedZoneId = zone->id;
                            previewRefreshRequested = processor.submitAuthoringPreviewCommand(audition);
                            if (!previewRefreshRequested)
                                failed.store(true, std::memory_order_release);
                        }
                    }
                }
            }

            processor.serviceMessageThreadWork();
            std::this_thread::yield();
        }
        messageComplete.store(true, std::memory_order_release);
    });

    std::thread uiThread([&]
    {
        while (!start.load(std::memory_order_acquire))
            std::this_thread::yield();

        std::uint64_t previousSequence = 0;
        std::size_t previousBlockCount = 0;
        while ((!audioComplete.load(std::memory_order_acquire)
                || !messageComplete.load(std::memory_order_acquire))
               && !failed.load(std::memory_order_acquire))
        {
            const auto snapshot = processor.getRealtimeSafetySnapshot();
            if (!snapshotIsCoherent(snapshot, previousSequence, previousBlockCount))
            {
                failed.store(true, std::memory_order_release);
                break;
            }
            previousSequence = snapshot.publicationSequence;
            previousBlockCount = snapshot.processBlockCount;
            uiPollCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();
        }
    });

    start.store(true, std::memory_order_release);
    audioThread.join();
    messageThread.join();
    uiThread.join();

    // The Release build can coalesce every in-flight draft change back to the
    // initially selected zone before the preparation worker publishes an
    // intermediate preview. Make the retirement portion of this test
    // deterministic instead of depending on Debug/Release scheduling speed.
    const auto postSoakPreviewSelected =
        processor.getAuthoringSession().selectZone("pad-a3-low").applied;
    auto postSoakZone = processor.getAuthoringSession().getSelectedZone();
    auto postSoakPreviewEdited = false;
    if (postSoakPreviewSelected && postSoakZone.has_value())
    {
        postSoakZone->pan = postSoakZone->pan >= 0.0 ? -0.375 : 0.375;
        postSoakPreviewEdited = processor.getAuthoringSession().updateSelectedZone(
            *postSoakZone, "Post-soak retirement edit").applied;
    }
    drs::engine::AuthoringPreviewCommand postSoakAudition;
    postSoakAudition.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
    postSoakAudition.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    postSoakAudition.midiNote = postSoakZone.has_value() ? postSoakZone->rootKey : 60;
    postSoakAudition.velocity = 0.68f;
    postSoakAudition.selectedZoneId = postSoakZone.has_value() ? postSoakZone->id : std::string {};
    const auto postSoakPreviewRequested = postSoakPreviewEdited
        && processor.submitAuthoringPreviewCommand(postSoakAudition);
    processor.serviceMessageThreadWork();
    juce::AudioBuffer<float> retirementBuffer(2, 1024);
    juce::MidiBuffer emptyMidi;
    auto postSoakBlockCount = 0;
    auto postSoakPreviewActivated = false;
    const auto activationDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < activationDeadline)
    {
        processor.serviceMessageThreadWork();
        retirementBuffer.clear();
        processor.processBlock(retirementBuffer, emptyMidi);
        ++postSoakBlockCount;
        processor.serviceMessageThreadWork();
        if (processor.getRealtimeSafetySnapshot().authoringPreviewActivationCount > 1)
        {
            postSoakPreviewActivated = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    processor.queuePerformanceSurfaceNoteOff(60);
    drs::engine::AuthoringPreviewCommand stopPreview;
    stopPreview.type = drs::engine::AuthoringPreviewCommandType::stopAll;
    stopPreview.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    const auto postSoakPreviewStopped = processor.submitAuthoringPreviewCommand(stopPreview);
    processor.serviceMessageThreadWork();
    constexpr int retirementBlockCount = 64;
    for (int block = 0; block < retirementBlockCount; ++block)
    {
        retirementBuffer.clear();
        processor.processBlock(retirementBuffer, emptyMidi);
        processor.serviceMessageThreadWork();
    }

    const auto snapshot = processor.getRealtimeSafetySnapshot();
    const auto passed = !failed.load(std::memory_order_acquire)
        && initialPreviewActivated
        && postSoakPreviewSelected
        && postSoakPreviewEdited
        && postSoakPreviewRequested
        && postSoakPreviewActivated
        && postSoakPreviewStopped
        && uiPollCount.load(std::memory_order_acquire) > 0
        && snapshot.processBlockCount
            == primingBlockCount + audioBlockCount + postSoakBlockCount + retirementBlockCount
        // Draft macro edits exercise Preview only. Performance must remain on the
        // controller-authorized last-known-good activation until a newer Publish.
        && snapshot.performanceActivationCount == 1
        && snapshot.authoringPreviewActivationCount > 1
        && snapshot.retiredActivationCount > 0
        && snapshot.reclaimedActivationPayloadCount > 0
        && snapshot.performancePeakActiveVoiceCount > 0
        && snapshot.authoringPreviewPeakActiveVoiceCount > 0
        && (snapshot.performancePeakReleasingVoiceCount
            + snapshot.authoringPreviewPeakReleasingVoiceCount) > 0
        && snapshot.performanceDroppedNoteCount == 0
        && snapshot.authoringPreviewDroppedNoteCount == 0
        && snapshot.getAudioThreadViolationCount() == 0;

    if (!passed)
    {
        const auto previewStatus = processor.getAuthoringPreviewStatusSnapshot();
        const auto previewController = processor.getAuthoringPreviewControllerSnapshot();
        std::cerr << "Sprint 4 concurrency soak failed: polls="
                  << uiPollCount.load(std::memory_order_acquire)
                  << ", blocks=" << snapshot.processBlockCount
                  << ", performance activations=" << snapshot.performanceActivationCount
                  << ", preview activations=" << snapshot.authoringPreviewActivationCount
                  << ", retired=" << snapshot.retiredActivationCount
                  << ", reclaimed=" << snapshot.reclaimedActivationPayloadCount
                  << ", performance peak=" << snapshot.performancePeakActiveVoiceCount
                  << ", preview peak=" << snapshot.authoringPreviewPeakActiveVoiceCount
                  << ", performance releasing peak=" << snapshot.performancePeakReleasingVoiceCount
                  << ", preview releasing peak=" << snapshot.authoringPreviewPeakReleasingVoiceCount
                  << ", performance dropped notes=" << snapshot.performanceDroppedNoteCount
                  << ", preview dropped notes=" << snapshot.authoringPreviewDroppedNoteCount
                  << ", violations=" << snapshot.getAudioThreadViolationCount()
                  << ", preview state=" << previewStatus.stateLabel
                  << ", preview failure=" << previewStatus.failureState
                  << ", draft revision=" << previewStatus.draftRevision
                  << ", requested revision=" << previewStatus.requestedRevision
                  << ", active revision=" << previewStatus.activeRevision
                  << ", controller requested=" << previewController.requestedCount
                  << ", controller launched=" << previewController.launchedCount
                  << ", controller accepted=" << previewController.acceptedCount
                  << ", controller activations=" << previewController.activationCount
                  << std::endl;
        return 1;
    }

    std::cout << "Sprint 4 concurrency soak passed: polls="
              << uiPollCount.load(std::memory_order_acquire)
              << ", blocks=" << snapshot.processBlockCount
              << ", activations="
              << snapshot.performanceActivationCount + snapshot.authoringPreviewActivationCount
              << ", reclaimed=" << snapshot.reclaimedActivationPayloadCount
              << std::endl;
    return 0;
}
