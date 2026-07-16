#pragma once

#include "shared/authoring/AuthoringViewModels.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class ZoneMappingEditor final : public juce::Component
{
public:
    enum class LayoutMode
    {
        compact,
        expanded
    };

    explicit ZoneMappingEditor(LayoutMode layoutMode);

    void resized() override;
    void setViewModel(ZoneFieldValuesViewModel nextViewModel);
    void setCallbacks(ZoneFieldCallbacks nextCallbacks);

private:
    ZoneFieldValuesViewModel collectCurrentValues() const;

    LayoutMode layoutMode = LayoutMode::compact;
    ZoneFieldValuesViewModel viewModel;
    ZoneFieldCallbacks callbacks;

    juce::Label emptyStateLabel;
    juce::Label rootKeyLabel;
    juce::Label keyLowLabel;
    juce::Label keyHighLabel;
    juce::Label velocityLowLabel;
    juce::Label velocityHighLabel;
    juce::Label gainLabel;
    juce::Label panLabel;

    juce::Slider rootKeySlider;
    juce::Slider keyLowSlider;
    juce::Slider keyHighSlider;
    juce::Slider velocityLowSlider;
    juce::Slider velocityHighSlider;
    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::ToggleButton loopEnabledToggle;
    juce::TextButton restoreRootKeyButton;
};
} // namespace drs::app::authoring
