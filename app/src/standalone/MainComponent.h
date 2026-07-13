#pragma once

#include "plugin/PluginProcessor.h"
#include "shared/PerformancePanel.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace drs::standalone
{
class MainComponent final : public juce::Component
{
public:
    explicit MainComponent(bool enableAudioOutput = true);
    ~MainComponent() override;

    void resized() override;

    std::string exportStateJson() const;
    drs::engine::EnginePresetStateRestoreResult restoreStateJson(const std::string& stateJson);
    bool setMacroValue(const std::string& macroId, double value);
    drs::engine::EngineFacade& getEngineFacade() { return processor.getEngineFacade(); }
    const drs::engine::EngineFacade& getEngineFacade() const { return processor.getEngineFacade(); }
    drs::plugin::Processor& getProcessor() { return processor; }
    const drs::plugin::Processor& getProcessor() const { return processor; }
    bool isAudioOutputEnabled() const { return audioOutputEnabled; }
    bool hasAudioDeviceError() const { return !audioDeviceError.isEmpty(); }
    const juce::String& getAudioDeviceError() const { return audioDeviceError; }

private:
    void initializeAudioOutput();
    void shutdownAudioOutput();

    drs::plugin::Processor processor;
    drs::app::PerformancePanel performancePanel;
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioProcessorPlayer audioProcessorPlayer;
    bool audioOutputEnabled = false;
    juce::String audioDeviceError;
};
} // namespace drs::standalone
