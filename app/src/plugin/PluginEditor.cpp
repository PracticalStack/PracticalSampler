#include "plugin/PluginEditor.h"

namespace drs::plugin
{
Editor::Editor(Processor& owner)
    : juce::AudioProcessorEditor(owner),
      processor(owner),
      statusPanel(owner.getEngineFacade())
{
    addAndMakeVisible(statusPanel);
    setSize(720, 420);
}

void Editor::resized()
{
    statusPanel.setBounds(getLocalBounds());
}
} // namespace drs::plugin
