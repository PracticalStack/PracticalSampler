#include "plugin/PluginEditor.h"

namespace drs::plugin
{
Editor::Editor(Processor& owner)
    : juce::AudioProcessorEditor(owner),
      processor(owner),
      performancePanel(owner.getEngineFacade(),
                       [&owner](const std::string& macroId, double value)
                       {
                           owner.setMacroValueFromShell(macroId, value);
                       },
                       [&owner](int midiNoteNumber, float velocity)
                       {
                           owner.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);
                       },
                       [&owner](int midiNoteNumber)
                       {
                           owner.queuePerformanceSurfaceNoteOff(midiNoteNumber);
                       })
{
    addAndMakeVisible(performancePanel);
    setSize(820, 700);
}

void Editor::resized()
{
    performancePanel.setBounds(getLocalBounds());
}
} // namespace drs::plugin
