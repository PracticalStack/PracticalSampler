#include "plugin/PluginEditor.h"

namespace drs::plugin
{
Editor::Editor(Processor& owner)
    : juce::AudioProcessorEditor(owner),
      processor(owner),
      statusPanel(owner.getEngineFacade(),
                  [&owner](const std::string& macroId, double value)
                  {
                      owner.setMacroValueFromShell(macroId, value);
                  })
{
    addAndMakeVisible(statusPanel);
    setSize(720, 420);
}

void Editor::resized()
{
    statusPanel.setBounds(getLocalBounds());
}
} // namespace drs::plugin
