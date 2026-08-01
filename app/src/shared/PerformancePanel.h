#pragma once

#include "drs/engine/EngineFacade.h"
#include "shared/PerformanceMixer.h"
#include "shared/StatusPanel.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <memory>
#include <vector>

namespace drs::app
{
class PerformancePanel final : public juce::Component,
                               private juce::Timer,
                               private juce::MidiKeyboardStateListener
{
public:
    using MacroValueChangedCallback = std::function<void(const std::string&, double)>;
    using PerformanceNoteOnCallback = std::function<void(int, float)>;
    using PerformanceNoteOffCallback = std::function<void(int)>;
    using PublishCommandCallback = StatusPanel::PublishCommandCallback;
    using PublishPresentationProvider = StatusPanel::PublishPresentationProvider;
    using AudioCallbackActiveProvider = std::function<bool()>;

    explicit PerformancePanel(drs::engine::EngineFacade& engineFacade,
                              MacroValueChangedCallback onMacroValueChanged = {},
                              PerformanceNoteOnCallback onPerformanceNoteOn = {},
                              PerformanceNoteOffCallback onPerformanceNoteOff = {},
                              PublishCommandCallback onPublishCommand = {},
                              PublishPresentationProvider publishPresentationProvider = {},
                              AudioCallbackActiveProvider audioCallbackActiveProvider = {});
    ~PerformancePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshNow();
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

private:
    struct MacroControl
    {
        std::string id;
        juce::Label nameLabel;
        juce::Slider slider;
        juce::Label valueLabel;
        bool mixerControl = false;
    };

    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void rebuildMacroControls(const std::vector<drs::engine::EngineMacroDescriptor>& macros,
                              bool mixerControl);
    void rebuildArticulationButtons();
    void refreshSurface();
    void syncKeyboardPlayableRange();

    drs::engine::EngineFacade& engineFacade;
    MacroValueChangedCallback onMacroValueChanged;
    PerformanceNoteOnCallback onPerformanceNoteOn;
    PerformanceNoteOffCallback onPerformanceNoteOff;
    PublishPresentationProvider publishPresentationProvider;
    AudioCallbackActiveProvider audioCallbackActiveProvider;
    bool hasActivePublishedPerformance = false;
    juce::String publishedPerformanceStateLabel { "Idle" };
    juce::String publishedPerformanceGuidance;
    juce::String publishedPerformanceFindingCode;
    drs::engine::EnginePerformanceSnapshot performanceSnapshot;
    std::uint64_t lastObservedStateRevision = 0;
    bool showingPublishedMixer = false;
    std::size_t hiddenPublishedMacroCount = 0;
    std::vector<std::string> visibleMacroIds;
    std::vector<std::unique_ptr<MacroControl>> macroControls;
    PerformanceMixer publishedMixer;
    std::vector<std::unique_ptr<juce::TextButton>> articulationButtons;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    StatusPanel diagnosticsPanel;

    juce::Label titleLabel;
    juce::Label instrumentLabel;
    juce::Label patchStatusLabel;
    juce::Label previewStatusLabel;
    juce::Label macroStripLabel;
    juce::Label mixerEmptyStateLabel;
    juce::Label articulationLabel;
    juce::Label keyboardHintLabel;
    juce::Label loadIndicatorLabel;
    juce::TextButton loadDefaultButton;
    juce::TextButton loadLeadButton;
    juce::ToggleButton diagnosticsToggle;
};
} // namespace drs::app
