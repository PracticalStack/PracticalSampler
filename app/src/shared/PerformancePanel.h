#pragma once

#include "drs/engine/EngineFacade.h"
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
    using NotePreviewStartedCallback = std::function<void(int, float)>;
    using NotePreviewEndedCallback = std::function<void(int)>;

    explicit PerformancePanel(drs::engine::EngineFacade& engineFacade,
                              MacroValueChangedCallback onMacroValueChanged = {},
                              NotePreviewStartedCallback onNotePreviewStarted = {},
                              NotePreviewEndedCallback onNotePreviewEnded = {});
    ~PerformancePanel() override;

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
    };

    void timerCallback() override;
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void rebuildMacroControls();
    void rebuildArticulationButtons();
    void refreshSurface();
    void syncKeyboardPlayableRange();

    drs::engine::EngineFacade& engineFacade;
    MacroValueChangedCallback onMacroValueChanged;
    NotePreviewStartedCallback onNotePreviewStarted;
    NotePreviewEndedCallback onNotePreviewEnded;
    drs::engine::EnginePerformanceSnapshot performanceSnapshot;
    std::uint64_t lastObservedStateRevision = 0;
    std::vector<std::unique_ptr<MacroControl>> macroControls;
    std::vector<std::unique_ptr<juce::TextButton>> articulationButtons;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;
    StatusPanel diagnosticsPanel;

    juce::Label titleLabel;
    juce::Label instrumentLabel;
    juce::Label patchStatusLabel;
    juce::Label previewStatusLabel;
    juce::Label macroStripLabel;
    juce::Label articulationLabel;
    juce::Label keyboardHintLabel;
    juce::Label loadIndicatorLabel;
    juce::TextButton loadDefaultButton;
    juce::TextButton loadLeadButton;
    juce::ToggleButton diagnosticsToggle;
};
} // namespace drs::app
