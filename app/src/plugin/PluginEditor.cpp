#include "plugin/PluginEditor.h"

#include "shared/authoring/AuthoringWorkspaceLayout.h"

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
                     drs::app::AuthoringPanel::LayoutMode::compact,
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
    setSize(drs::app::authoring::compactShellWidth, drs::app::authoring::compactShellHeight);
}

void Editor::resized()
{
    workspaceTabs.setBounds(getLocalBounds());
}
} // namespace drs::plugin
