#pragma once

#include "drs/engine/EngineFacade.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>
#include <vector>

namespace drs::app
{
class StatusPanel final : public juce::Component,
                          private juce::Timer
{
public:
    using MacroValueChangedCallback = std::function<void(const std::string&, double)>;

    explicit StatusPanel(drs::engine::EngineFacade& engineFacade,
                         MacroValueChangedCallback onMacroValueChanged = {});

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct MacroControl
    {
        std::string id;
        juce::Label nameLabel;
        juce::Slider slider;
        juce::Label valueLabel;
        double minValue = 0.0;
        double maxValue = 1.0;
    };

    void timerCallback() override;
    void rebuildMacroControls();
    void refreshSnapshot();

    drs::engine::EngineFacade& engineFacade;
    drs::engine::EngineStatusSnapshot snapshot;
    MacroValueChangedCallback onMacroValueChanged;
    std::vector<std::unique_ptr<MacroControl>> macroControls;

    juce::Label titleLabel;
    juce::Label modeLabel;
    juce::Label stateLabel;
    juce::Label diagnosticsHeadlineLabel;
    juce::Label sessionLabel;
    juce::Label voicesLabel;
    juce::Label cacheLabel;
    juce::Label latencyLabel;
    juce::Label failureLabel;
    juce::Label routedZonesLabel;
    juce::Label actionsLabel;
    juce::Label draftPlaybackLabel;
    juce::Label macrosLabel;
    juce::TextButton resetStateButton;
    juce::TextButton loadLeadFixtureButton;
    juce::TextButton injectInvalidStateButton;
    juce::TextButton stageDraftButton;
    juce::TextButton preparePreviewButton;
    juce::TextButton publishDraftButton;
    juce::Label contentProbeLabel;
    juce::TextButton probeMissingContentButton;
    juce::TextButton probeBadChecksumButton;
    juce::TextButton probeSchemaMismatchButton;
    juce::TextButton probePartialArtifactButton;
    juce::TextButton clearProbeButton;
    juce::TextEditor detailEditor;
    juce::Label nextStepsLabel;
    juce::TextEditor nextStepsEditor;
};
} // namespace drs::app
