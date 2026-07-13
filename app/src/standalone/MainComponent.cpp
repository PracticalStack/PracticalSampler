#include "standalone/MainComponent.h"

namespace drs::standalone
{
MainComponent::MainComponent(bool enableAudioOutput)
    : performancePanel(processor.getEngineFacade(),
                       [this](const std::string& macroId, double value)
                       {
                           processor.setMacroValueFromShell(macroId, value);
                       },
                       [this](int midiNoteNumber, float velocity)
                       {
                           processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                       },
                       [this](int midiNoteNumber)
                       {
                           processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                       }),
      authoringPanel(processor.getAuthoringSession(),
                     [this]()
                     {
                         return processor.getAuthoringWaveformPreview();
                     },
                     [this]()
                     {
                         return processor.getAuthoringImportResponsivenessSnapshot();
                     },
                     drs::app::AuthoringPanel::LayoutMode::expanded,
                     [this](int midiNoteNumber, float velocity)
                     {
                         processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                     },
                     [this](int midiNoteNumber)
                     {
                         processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                     })
{
    workspaceTabs.setComponentID("workspaceTabs");
    workspaceTabs.addTab("Perform", juce::Colour::fromRGB(28, 126, 214), &performancePanel, false);
    workspaceTabs.addTab("Map", juce::Colour::fromRGB(181, 96, 21), &authoringPanel, false);
    addAndMakeVisible(workspaceTabs);
    setSize(860, 760);

    if (enableAudioOutput)
        initializeAudioOutput();
}

MainComponent::~MainComponent()
{
    shutdownAudioOutput();
}

void MainComponent::resized()
{
    workspaceTabs.setBounds(getLocalBounds());
}

std::string MainComponent::exportStateJson() const
{
    juce::MemoryBlock stateBlock;
    const_cast<drs::plugin::Processor&>(processor).getStateInformation(stateBlock);
    return std::string(static_cast<const char*>(stateBlock.getData()), stateBlock.getSize());
}

drs::engine::EnginePresetStateRestoreResult MainComponent::restoreStateJson(const std::string& stateJson)
{
    const auto validationResult = processor.getEngineFacade().restorePresetStateJson(stateJson);
    if (!validationResult.restored)
        return validationResult;

    processor.setStateInformation(stateJson.data(), static_cast<int>(stateJson.size()));
    return validationResult;
}

bool MainComponent::setMacroValue(const std::string& macroId, double value)
{
    const auto parameterId = "macro." + juce::String::fromUTF8(macroId.c_str());
    if (processor.getParameterState().getParameter(parameterId) == nullptr)
        return false;

    processor.setMacroValueFromShell(macroId, value);
    return true;
}

void MainComponent::initializeAudioOutput()
{
    audioProcessorPlayer.setProcessor(&processor);

    const auto setupError = audioDeviceManager.initialise(0, 2, nullptr, true);
    if (setupError.isNotEmpty())
    {
        audioDeviceError = setupError;
        audioProcessorPlayer.setProcessor(nullptr);
        return;
    }

    audioDeviceManager.addAudioCallback(&audioProcessorPlayer);
    audioOutputEnabled = true;
}

void MainComponent::shutdownAudioOutput()
{
    if (audioOutputEnabled)
        audioDeviceManager.removeAudioCallback(&audioProcessorPlayer);

    audioProcessorPlayer.setProcessor(nullptr);
    audioOutputEnabled = false;
}
} // namespace drs::standalone
