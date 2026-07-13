#pragma once

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
    drs::app::PerformancePanel performancePanel;
};
} // namespace drs::plugin
