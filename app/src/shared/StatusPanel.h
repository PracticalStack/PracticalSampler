#pragma once

#include "drs/engine/EngineFacade.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace drs::app
{
class StatusPanel final : public juce::Component,
                          private juce::Timer
{
public:
    explicit StatusPanel(drs::engine::EngineFacade& engineFacade);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshSnapshot();

    drs::engine::EngineFacade& engineFacade;
    drs::engine::EngineStatusSnapshot snapshot;

    juce::Label titleLabel;
    juce::Label modeLabel;
    juce::Label stateLabel;
    juce::TextEditor detailEditor;
    juce::Label nextStepsLabel;
    juce::TextEditor nextStepsEditor;
};
} // namespace drs::app
