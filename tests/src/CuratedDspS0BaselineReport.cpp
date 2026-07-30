#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <fstream>
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

void writeReport(const std::string& path,
                 const drs::plugin::ProcessorRealtimeSafetySnapshot& snapshot)
{
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not write baseline report: " + path);
    output << "{\n"
           << "  \"schema\": \"drs.curatedDsp.s0Baseline\",\n"
           << "  \"sampleRate\": 48000,\n"
           << "  \"blockFrames\": 512,\n"
           << "  \"renderedBlocks\": " << snapshot.processBlockCount << ",\n"
           << "  \"callbackBudgetMicros\": " << snapshot.callbackBudgetMicros << ",\n"
           << "  \"lastProcessBlockMicros\": " << snapshot.lastProcessBlockMicros << ",\n"
           << "  \"maxProcessBlockMicros\": " << snapshot.maxProcessBlockMicros << ",\n"
           << "  \"overBudgetCallbackCount\": " << snapshot.overBudgetCallbackCount << ",\n"
           << "  \"activePreparedBytes\": " << snapshot.activeActivationPayloadBytes << ",\n"
           << "  \"retiredPreparedBytes\": " << snapshot.retiredActivationPayloadBytes << ",\n"
           << "  \"realtimeGuardFailures\": " << snapshot.getRealtimeGuardFailureCount() << "\n"
           << "}\n";
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: drs_curated_dsp_s0_baseline_report <output-json>");
        juce::ScopedJuceInitialiser_GUI gui;
        drs::plugin::Processor processor;
        processor.prepareToPlay(48000.0, 512);
        processor.getEngineFacade().resetSessionStateToDefault();
        require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Default no-DSP activation did not prepare.");
        require(processor.serviceMessageThreadWork(),
                "Default no-DSP activation did not reach the processor.");

        juce::AudioBuffer<float> output(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        processor.processBlock(output, midi);
        require(output.getMagnitude(0, output.getNumSamples()) > 0.0001f,
                "No-DSP baseline must render audible output.");

        for (int block = 0; block < 255; ++block)
        {
            output.clear();
            processor.processBlock(output, midi);
            midi.clear();
        }

        const auto snapshot = processor.getRealtimeSafetySnapshot();
        require(snapshot.processBlockCount >= 256, "Baseline did not process the requested callback count.");
        require(snapshot.activeActivationPayloadBytes > 0,
                "Baseline must retain the active immutable playback payload.");
        require(snapshot.getAudioThreadViolationCount() == 0,
                "No-DSP baseline recorded prohibited audio-thread work.");
        writeReport(argv[1], snapshot);
        std::cout << "Curated DSP S0 baseline report passed. callback max="
                  << snapshot.maxProcessBlockMicros << "us, active payload="
                  << snapshot.activeActivationPayloadBytes << " bytes." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP S0 baseline report failed: " << exception.what() << std::endl;
        return 1;
    }
}
