#pragma once

#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/SampleImport.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <unordered_map>
#include <vector>

namespace drs::plugin
{
class Processor final : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    Processor();
    ~Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    drs::engine::EngineFacade& getEngineFacade() { return engineFacade; }
    const drs::engine::EngineFacade& getEngineFacade() const { return engineFacade; }
    juce::AudioProcessorValueTreeState& getParameterState() { return parameterState; }
    const juce::AudioProcessorValueTreeState& getParameterState() const { return parameterState; }
    void setMacroValueFromShell(const std::string& macroId, double value);
    void queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity);
    void queuePerformanceSurfaceNoteOff(int midiNoteNumber);

private:
    struct LoadedReferenceSample
    {
        drs::engine::ImportedSampleData sample;
    };

    struct ActiveRenderVoice
    {
        std::uint64_t renderVoiceId = 0;
        int sourceMidiNote = 0;
        int effectiveMidiNote = 0;
        int effectiveVelocity = 0;
        int rootKey = 60;
        std::string zoneId;
        std::string sampleId;
        const LoadedReferenceSample* loadedSample = nullptr;
        double positionFrames = 0.0;
        double incrementFrames = 1.0;
        float baseGain = 0.0f;
        bool releasing = false;
        int releaseSamplesRemaining = 0;
        int releaseSamplesTotal = 0;
    };

    static juce::String buildMacroParameterId(const std::string& macroId);
    static juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout(
        const drs::engine::EngineFacade& engineFacade);
    void initializeReferencePlaybackAssets();
    void startVoiceForMidiMessage(const juce::MidiMessage& message);
    void releaseVoicesForMidiNote(int midiNoteNumber);
    void renderBlockRange(juce::AudioBuffer<float>& buffer, int startSample, int sampleCount);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncEngineFromParameters();
    void syncParametersFromEngine();

    drs::engine::EngineFacade engineFacade;
    drs::engine::RuntimeManifestLoadResult referenceManifest;
    drs::engine::RuntimeStreamLoadResult referenceStream;
    std::unordered_map<std::string, LoadedReferenceSample> loadedSamples;
    std::vector<ActiveRenderVoice> activeVoices;
    juce::MidiMessageCollector performanceSurfaceMidiCollector;
    juce::AudioProcessorValueTreeState parameterState;
    double currentSampleRate = 44100.0;
    std::uint64_t nextRenderVoiceId = 1;
    bool isSynchronizingParameterState = false;
};
} // namespace drs::plugin
