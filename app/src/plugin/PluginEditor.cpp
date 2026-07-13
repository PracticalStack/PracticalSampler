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
                       }),
      authoringPanel(owner.getAuthoringSession(),
                     [&owner]()
                     {
                         return owner.getAuthoringWaveformPreview();
                     },
                     [&owner]()
                     {
                         return owner.getAuthoringImportResponsivenessSnapshot();
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
    workspaceTabs.setComponentID("workspaceTabs");
    workspaceTabs.addTab("Perform", juce::Colour::fromRGB(28, 126, 214), &performancePanel, false);
    workspaceTabs.addTab("Map", juce::Colour::fromRGB(181, 96, 21), &authoringPanel, false);
    addAndMakeVisible(workspaceTabs);
    setSize(820, 700);
}

void Editor::resized()
{
    workspaceTabs.setBounds(getLocalBounds());
}
} // namespace drs::plugin
