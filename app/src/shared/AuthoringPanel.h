#pragma once

#include "shared/AuthoringPreviewModel.h"
#include "shared/PerformanceBankImport.h"
#include "drs/engine/AuthoringSession.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <vector>

namespace drs::app
{
class AuthoringPanel final : public juce::Component
{
public:
    enum class LayoutMode
    {
        compact,
        expanded
    };

    using NotePreviewStartedCallback = std::function<void(int, float)>;
    using NotePreviewEndedCallback = std::function<void(int)>;
    using WaveformPreviewProvider = std::function<AuthoringWaveformPreview()>;
    using ImportResponsivenessProvider = std::function<AuthoringImportResponsivenessSnapshot()>;

    explicit AuthoringPanel(drs::engine::AuthoringSession& authoringSession,
                            WaveformPreviewProvider waveformPreviewProvider = {},
                            ImportResponsivenessProvider importResponsivenessProvider = {},
                            LayoutMode layoutMode = LayoutMode::compact,
                            NotePreviewStartedCallback onNotePreviewStarted = {},
                            NotePreviewEndedCallback onNotePreviewEnded = {});

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class ZoneMapComponent final : public juce::Component
    {
    public:
        void setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries);
        void paint(juce::Graphics& g) override;

    private:
        std::vector<drs::engine::AuthoringZoneSummary> zoneSummaries;
    };

    class WaveformPreviewComponent final : public juce::Component
    {
    public:
        void setPreview(AuthoringWaveformPreview preview);
        void paint(juce::Graphics& g) override;

    private:
        AuthoringWaveformPreview preview;
    };

    void rebuildZoneSelector();
    void rebuildMacroSelector();
    void rebuildFxSelector();
    void rebuildRoutingBusSelector();
    void rebuildPerformanceBankSelector();
    void rebuildTriggerSlotSelector();
    void refreshInspectorVisibility();
    void refreshFromSession();
    void applySelectedZoneEdit(const juce::String& label);
    void applySelectedMacroEdit(const juce::String& label);
    void moveSelectedMacro(int direction);
    void applySelectedFxSlotEdit(const juce::String& label);
    void applySelectedRoutingBusEdit(const juce::String& label);
    void applySelectedTriggerSlotEdit(const juce::String& label);
    void importPhraseForSelectedBank();

    drs::engine::AuthoringSession& authoringSession;
    WaveformPreviewProvider waveformPreviewProvider;
    ImportResponsivenessProvider importResponsivenessProvider;
    LayoutMode layoutMode = LayoutMode::compact;
    NotePreviewStartedCallback onNotePreviewStarted;
    NotePreviewEndedCallback onNotePreviewEnded;
    bool isRefreshing = false;
    int selectedMacroIndex = 0;
    int selectedFxSlotIndex = 0;
    int selectedRoutingBusIndex = 0;
    int selectedPerformanceBankIndex = 0;
    int selectedTriggerSlotIndex = 0;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label sourceLabel;
    juce::Label articulationLabel;
    juce::Label waveformLabel;
    juce::Label waveformInfoLabel;
    juce::Label loopInfoLabel;
    juce::Label importMetricsLabel;
    juce::Label inspectorModeLabel;
    juce::ComboBox inspectorModeSelector;
    juce::Label zoneLabel;
    juce::ComboBox zoneSelector;
    ZoneMapComponent zoneMap;
    WaveformPreviewComponent waveformPreview;

    juce::Slider rootKeySlider;
    juce::Slider keyLowSlider;
    juce::Slider keyHighSlider;
    juce::Slider velocityLowSlider;
    juce::Slider velocityHighSlider;
    juce::Slider gainSlider;
    juce::Slider panSlider;

    juce::Label rootKeyLabel;
    juce::Label keyLowLabel;
    juce::Label keyHighLabel;
    juce::Label velocityLowLabel;
    juce::Label velocityHighLabel;
    juce::Label gainLabel;
    juce::Label panLabel;

    juce::Label macroSectionLabel;
    juce::ComboBox macroSelector;
    juce::Label macroAssignmentLabel;
    juce::ComboBox macroAssignmentSelector;
    juce::Label macroRoleLabel;
    juce::ComboBox macroRoleSelector;
    juce::Label macroDefaultLabel;
    juce::Slider macroDefaultSlider;
    juce::Label macroMinLabel;
    juce::Slider macroMinSlider;
    juce::Label macroMaxLabel;
    juce::Slider macroMaxSlider;
    juce::Label macroSummaryLabel;
    juce::TextButton macroMoveUpButton;
    juce::TextButton macroMoveDownButton;

    juce::Label fxSectionLabel;
    juce::ComboBox fxSelector;
    juce::Label fxTypeLabel;
    juce::ComboBox fxTypeSelector;
    juce::ToggleButton fxBypassedToggle;
    juce::Label fxSummaryLabel;

    juce::Label routingSectionLabel;
    juce::ComboBox routingBusSelector;
    juce::Label routingInputLabel;
    juce::ComboBox routingInputSelector;
    juce::Label routingInsertOneLabel;
    juce::ComboBox routingInsertOneSelector;
    juce::Label routingInsertTwoLabel;
    juce::ComboBox routingInsertTwoSelector;
    juce::Label routingSummaryLabel;

    juce::Label performanceSectionLabel;
    juce::ComboBox performanceBankSelector;
    juce::ComboBox triggerSlotSelector;
    juce::Label triggerEventLabel;
    juce::ComboBox triggerEventSelector;
    juce::Label targetArticulationLabel;
    juce::ComboBox targetArticulationSelector;
    juce::Label phraseAssetLabel;
    juce::ComboBox phraseAssetSelector;
    juce::Label chordModeLabel;
    juce::ComboBox chordModeSelector;
    juce::Label phraseImportPathLabel;
    juce::TextEditor phraseImportPathEditor;
    juce::TextButton phraseImportButton;
    juce::Label performanceSummaryLabel;
    juce::Label phraseSummaryLabel;

    juce::ToggleButton loopEnabledToggle;
    juce::TextButton previewButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton saveCheckpointButton;
};
} // namespace drs::app
