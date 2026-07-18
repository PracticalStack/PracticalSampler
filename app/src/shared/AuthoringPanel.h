#pragma once

#include "shared/authoring/AuthoringSummaryStrip.h"
#include "shared/authoring/AuthoringViewModels.h"
#include "shared/authoring/RepeatedStructureList.h"
#include "shared/authoring/ZoneMappingEditor.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "shared/authoring/WaveformDetailView.h"
#include "shared/AuthoringPreviewModel.h"
#include "shared/PerformanceBankImport.h"
#include "drs/engine/AuthoringSession.h"
#include "drs/engine/DraftPlaybackContract.h"

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
    using AuthoringPreviewStatusProvider = std::function<AuthoringPreviewStatusSnapshot()>;
    using ImportResponsivenessProvider = std::function<AuthoringImportResponsivenessSnapshot()>;
    using RestoreRootKeyCallback = std::function<void()>;
    using DraftPlaybackStatusProvider = std::function<drs::engine::DraftPlaybackStatus()>;

    explicit AuthoringPanel(drs::engine::AuthoringSession& authoringSession,
                            WaveformPreviewProvider waveformPreviewProvider = {},
                            AuthoringPreviewStatusProvider authoringPreviewStatusProvider = {},
                            ImportResponsivenessProvider importResponsivenessProvider = {},
                            LayoutMode layoutMode = LayoutMode::compact,
                            NotePreviewStartedCallback onNotePreviewStarted = {},
                            NotePreviewEndedCallback onNotePreviewEnded = {},
                            RestoreRootKeyCallback onRestoreRootKeyRequested = {},
                            DraftPlaybackStatusProvider draftPlaybackStatusProvider = {});
    ~AuthoringPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void reloadFromSession();

private:
    class AuthoringControlLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        AuthoringControlLookAndFeel();

        void drawButtonBackground(juce::Graphics& g,
                                  juce::Button& button,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;
        void drawToggleButton(juce::Graphics& g,
                              juce::ToggleButton& button,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
        void drawComboBox(juce::Graphics& g,
                          int width,
                          int height,
                          bool isButtonDown,
                          int buttonX,
                          int buttonY,
                          int buttonW,
                          int buttonH,
                          juce::ComboBox& box) override;
        void drawLinearSliderOutline(juce::Graphics& g,
                                     int x,
                                     int y,
                                     int width,
                                     int height,
                                     const juce::Slider::SliderStyle style,
                                     juce::Slider& slider) override;
    };

    void rebuildZoneSelector();
    void rebuildMacroList();
    void rebuildFxSelector();
    void rebuildRoutingBusSelector();
    void rebuildPerformanceBankSelector();
    void rebuildTriggerSlotSelector();
    void refreshInspectorVisibility();
    void refreshDrawerContextLabels();
    void refreshContextualAccessibility();
    void refreshWaveformDrawerContent();
    void refreshFromSession();
    void applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values, const juce::String& label);
    void applySelectedMacroEdit(const juce::String& label);
    void moveSelectedMacro(int direction);
    void applySelectedFxSlotEdit(const juce::String& label);
    void applySelectedRoutingBusEdit(const juce::String& label);
    void applySelectedTriggerSlotEdit(const juce::String& label);
    void importPhraseForSelectedBank();
    void previewSelectedZone();
    void undoLastEdit();
    void redoLastEdit();
    void markSavedCheckpoint();
    void setDrawerOpen(bool shouldOpen);
    void setActiveDrawerTab(authoring::DrawerTab nextTab);
    void configureAccessibilityAndFocus();
    void refreshDrawerVisibility();
    authoring::SelectionSummaryViewModel buildSelectionSummaryViewModel() const;
    authoring::ZoneFieldValuesViewModel buildZoneFieldValuesViewModel() const;

    drs::engine::AuthoringSession& authoringSession;
    WaveformPreviewProvider waveformPreviewProvider;
    AuthoringPreviewStatusProvider authoringPreviewStatusProvider;
    ImportResponsivenessProvider importResponsivenessProvider;
    LayoutMode layoutMode = LayoutMode::compact;
    NotePreviewStartedCallback onNotePreviewStarted;
    NotePreviewEndedCallback onNotePreviewEnded;
    RestoreRootKeyCallback onRestoreRootKeyRequested;
    DraftPlaybackStatusProvider draftPlaybackStatusProvider;
    bool isRefreshing = false;
    int selectedMacroIndex = 0;
    int selectedFxSlotIndex = 0;
    int selectedRoutingBusIndex = 0;
    int selectedPerformanceBankIndex = 0;
    int selectedTriggerSlotIndex = 0;
    authoring::DrawerState drawerState;
    authoring::SelectionSummaryViewModel selectionSummaryViewModel;
    authoring::ZoneFieldValuesViewModel zoneFieldValuesViewModel;
    AuthoringControlLookAndFeel authoringLookAndFeel;

    authoring::AuthoringSummaryStrip summaryStrip;
    juce::Label waveformLabel;
    juce::Label waveformScopeLabel;
    juce::Label drawerBreadcrumbLabel;
    juce::Label waveformStatusLabel;
    juce::Label waveformInfoLabel;
    juce::Label loopInfoLabel;
    juce::Label importMetricsLabel;
    juce::Component drawerRegion;
    juce::Component drawerTabStrip;
    juce::Component drawerContentHost;
    juce::TextButton drawerToggleButton;
    juce::TextButton drawerWaveformTabButton;
    juce::TextButton drawerMacrosTabButton;
    juce::TextButton drawerRoutingTabButton;
    juce::TextButton drawerPerformanceTabButton;
    juce::Label zoneLabel;
    juce::ComboBox zoneSelector;
    authoring::ZoneMapCanvas zoneMap;
    authoring::ZoneMappingEditor zoneMappingEditor;
    authoring::WaveformDetailView waveformPreview;

    authoring::RepeatedStructureList macroList;
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

};
} // namespace drs::app
