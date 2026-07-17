#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

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
} // namespace

int main()
{
    try
    {
        drs::plugin::Processor processor;
        processor.prepareToPlay(44100.0, 512);

        const auto primedSnapshot = processor.getRealtimeSafetySnapshot();
        require(primedSnapshot.available, "Realtime safety snapshot must be available.");
        require(primedSnapshot.preparedBlockSize == 512, "Realtime safety snapshot should remember the prepared block size.");
        require(primedSnapshot.referenceSampleCountLoaded >= 1,
                "Performance playback samples should be preloaded before the callback runs.");
        require(primedSnapshot.referenceWarmupCount >= 1,
                "Realtime safety snapshot should record an off-audio-thread warmup pass.");
        require(primedSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "Preparing the processor should keep reference sample loading off the audio thread.");
        require(primedSnapshot.activeVoiceCapacity >= primedSnapshot.activeVoiceCapacityLimit,
                "Active-voice storage should be reserved before realtime playback begins.");
        require(primedSnapshot.getAudioThreadViolationCount() == 0,
                "Primed processor should not report realtime-thread safety violations.");

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer hostMidi;
        hostMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        processor.processBlock(buffer, hostMidi);

        require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0001f,
                "Host MIDI note-on should render audible output through the performance path.");

        auto playbackSnapshot = processor.getRealtimeSafetySnapshot();
        require(playbackSnapshot.processBlockCount >= 1, "Realtime safety snapshot should count processed callbacks.");
        require(playbackSnapshot.callbackBudgetMicros > 0,
                "Realtime safety snapshot should expose a non-zero callback budget.");
        require(playbackSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "First rendered note should not trigger reference sample I/O on the audio thread.");
        require(playbackSnapshot.activeVoiceCapacityGrowthCount == 0,
                "Voice allocation should not force the active-voice vector to grow in the callback.");
        require(playbackSnapshot.getAudioThreadViolationCount() == 0,
                "Performance callback should remain free of tracked realtime safety violations.");

        for (int index = 0; index < 32; ++index)
            processor.queuePerformanceSurfaceNoteOn(57 + (index % 3), 0.75f);

        juce::AudioBuffer<float> queuedBuffer(2, 512);
        queuedBuffer.clear();
        juce::MidiBuffer emptyMidi;
        processor.processBlock(queuedBuffer, emptyMidi);

        require(queuedBuffer.getMagnitude(0, queuedBuffer.getNumSamples()) > 0.0001f,
                "Queued performance-surface notes should render audible output.");

        playbackSnapshot = processor.getRealtimeSafetySnapshot();
        require(playbackSnapshot.processBlockCount >= 2,
                "Realtime safety snapshot should continue counting later callbacks.");
        require(playbackSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "Queued performance playback should keep reference sample I/O off the callback thread.");
        require(playbackSnapshot.activeVoiceCapacityGrowthCount == 0,
                "Burst note starts should respect the pre-reserved active-voice capacity.");
        require(playbackSnapshot.getAudioThreadViolationCount() == 0,
                "Tracked realtime safety violations should remain at zero after burst playback.");

        std::cout << "Phase 1 realtime safety tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 realtime safety tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
