#pragma once

#include "drs/engine/EngineFacade.h"
#include "drs/engine/PerformancePublishCommandAdapter.h"

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
    using PublishCommandCallback = std::function<bool(
        const drs::engine::PerformancePublishCommand&,
        drs::engine::PerformancePublishCommandSource)>;
    using PublishPresentationProvider = std::function<
        std::shared_ptr<const drs::engine::PerformancePublishPresentationSnapshot>()>;

    explicit StatusPanel(drs::engine::EngineFacade& engineFacade,
                         MacroValueChangedCallback onMacroValueChanged = {},
                         PublishCommandCallback onPublishCommand = {},
                         PublishPresentationProvider publishPresentationProvider = {});

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshNow();

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
    PublishCommandCallback onPublishCommand;
    PublishPresentationProvider publishPresentationProvider;
    std::shared_ptr<const drs::engine::PerformancePublishPresentationSnapshot> publishPresentation;
    std::vector<std::unique_ptr<MacroControl>> macroControls;
    std::uint64_t lastObservedStateRevision = 0;
    std::uint64_t lastObservedPublicationSequence = 0;

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
