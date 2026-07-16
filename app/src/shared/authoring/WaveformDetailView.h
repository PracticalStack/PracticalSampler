#pragma once

#include "shared/AuthoringPreviewModel.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class WaveformDetailView final : public juce::Component
{
public:
    void setPreview(AuthoringWaveformPreview nextPreview);
    void paint(juce::Graphics& g) override;

private:
    AuthoringWaveformPreview preview;
};
} // namespace drs::app::authoring
