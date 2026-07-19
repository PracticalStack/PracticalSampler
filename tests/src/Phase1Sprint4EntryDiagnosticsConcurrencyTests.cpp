#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <string>
#include <thread>

namespace
{
bool isKnownProcessorState(const std::string& state)
{
    return state == "Realtime callback primed"
        || state == "Realtime callback violations recorded"
        || state == "Reference playback cache unavailable"
        || state == "Published activation pending"
        || state == "Authoring preview activation pending"
        || state == "Published activation unavailable";
}

bool isKnownPreviewState(const std::string& state)
{
    return state == "Idle" || state == "Preparing" || state == "Ready"
        || state == "Stale" || state == "Failed";
}

bool snapshotIsCoherent(const drs::plugin::ProcessorRealtimeSafetySnapshot& snapshot,
                        std::uint64_t previousSequence,
                        std::size_t previousProcessBlockCount)
{
    if (!snapshot.available || (snapshot.publicationSequence & 1u) != 0)
        return false;
    if (snapshot.publicationSequence < previousSequence
        || snapshot.processBlockCount < previousProcessBlockCount)
        return false;
    if (snapshot.performanceActiveVoiceCount + snapshot.authoringPreviewActiveVoiceCount
        > snapshot.activeVoiceCapacityLimit)
        return false;
    if (snapshot.activeVoiceCapacity < snapshot.performanceActiveVoiceCount
                                           + snapshot.authoringPreviewActiveVoiceCount)
        return false;
    if (snapshot.pendingPreparedBuildId == 0 && snapshot.pendingPublishedRevision != 0)
        return false;
    if (snapshot.retiredActivationBacklog == 0 && snapshot.retiredActivationPayloadBytes != 0)
        return false;
    if (!isKnownProcessorState(snapshot.state) || !isKnownPreviewState(snapshot.authoringPreviewRevisionState))
        return false;
    return true;
}
} // namespace

int main()
{
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 64);

    constexpr int audioBlockCount = 1200;
    constexpr int messageIterationCount = 360;
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
            const auto tone = static_cast<double>(iteration % 101) / 100.0;
            const auto motion = static_cast<double>((iteration * 7) % 101) / 100.0;
            processor.getEngineFacade().setMacroValue("tone", tone);
            processor.getEngineFacade().setMacroValue("motion", motion);

            if ((iteration % 12) == 0)
                processor.queuePerformanceSurfaceNoteOn(57 + ((iteration / 12) % 3), 0.72f);
            if ((iteration % 12) == 6)
                processor.queuePerformanceSurfaceNoteOff(57 + ((iteration / 12) % 3));

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
        std::size_t previousProcessBlockCount = 0;
        while ((!audioComplete.load(std::memory_order_acquire)
                || !messageComplete.load(std::memory_order_acquire))
               && !failed.load(std::memory_order_acquire))
        {
            const auto snapshot = processor.getRealtimeSafetySnapshot();
            if (!snapshotIsCoherent(snapshot, previousSequence, previousProcessBlockCount))
            {
                failed.store(true, std::memory_order_release);
                break;
            }

            previousSequence = snapshot.publicationSequence;
            previousProcessBlockCount = snapshot.processBlockCount;
            uiPollCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();
        }
    });

    start.store(true, std::memory_order_release);
    audioThread.join();
    messageThread.join();
    uiThread.join();

    processor.serviceMessageThreadWork();
    const auto finalSnapshot = processor.getRealtimeSafetySnapshot();
    const auto passed = !failed.load(std::memory_order_acquire)
        && uiPollCount.load(std::memory_order_acquire) > 0
        && finalSnapshot.processBlockCount == audioBlockCount
        && finalSnapshot.getAudioThreadViolationCount() == 0
        && (finalSnapshot.publicationSequence & 1u) == 0;

    if (!passed)
    {
        std::cerr << "EG3 diagnostics concurrency matrix failed: polls="
                  << uiPollCount.load(std::memory_order_acquire)
                  << ", blocks=" << finalSnapshot.processBlockCount
                  << ", sequence=" << finalSnapshot.publicationSequence
                  << ", violations=" << finalSnapshot.getAudioThreadViolationCount()
                  << std::endl;
        return 1;
    }

    std::cout << "Sprint 4 Entry Gate EG3 diagnostics concurrency matrix passed: polls="
              << uiPollCount.load(std::memory_order_acquire)
              << ", blocks=" << finalSnapshot.processBlockCount
              << ", sequence=" << finalSnapshot.publicationSequence
              << std::endl;
    return 0;
}
