#pragma once

#include "shared/AuthoringPanel.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePanel.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace drs::plugin
{
class Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor&);

    void resized() override;

private:
    Processor& processor;
    juce::TabbedComponent workspaceTabs { juce::TabbedButtonBar::TabsAtTop };
    drs::app::PerformancePanel performancePanel;
    drs::app::AuthoringPanel authoringPanel;
};
} // namespace drs::plugin
