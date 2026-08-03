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
#include "drs/engine/AuthoringPreviewCommandAdapter.h"
#include "drs/engine/DraftPlaybackContract.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace drs::app
{
class AuthoringPanel final : public juce::Component,
                             private juce::MultiTimer
{
public:
    enum class LayoutMode
    {
        compact,
        expanded
    };

    using WaveformPreviewProvider = std::function<AuthoringWaveformPreview()>;
    using WaveformPreviewRequestCallback = std::function<void()>;
    using AuthoringPreviewStatusProvider = std::function<AuthoringPreviewStatusSnapshot()>;
    using ImportResponsivenessProvider = std::function<AuthoringImportResponsivenessSnapshot()>;
    using SourceValidationStatusProvider = std::function<AuthoringSourceValidationSnapshot()>;
    using RestoreRootKeyCallback = std::function<void()>;
    using DraftPlaybackStatusProvider = std::function<drs::engine::DraftPlaybackStatus()>;
    using DraftPlaybackActionCallback = std::function<void()>;
    using PreviewCommandCallback = std::function<void(const drs::engine::AuthoringPreviewCommand&)>;
    using SampleFilesDroppedCallback = std::function<void(std::vector<juce::File>)>;

    explicit AuthoringPanel(drs::engine::AuthoringSession& authoringSession,
                            WaveformPreviewProvider waveformPreviewProvider = {},
                            AuthoringPreviewStatusProvider authoringPreviewStatusProvider = {},
                            ImportResponsivenessProvider importResponsivenessProvider = {},
                            LayoutMode layoutMode = LayoutMode::compact,
                            RestoreRootKeyCallback onRestoreRootKeyRequested = {},
                            DraftPlaybackStatusProvider draftPlaybackStatusProvider = {},
                            DraftPlaybackActionCallback onPrepareDraftPlaybackRequested = {},
                            DraftPlaybackActionCallback onPublishDraftPlaybackRequested = {},
                            PreviewCommandCallback previewCommandCallback = {},
                            SampleFilesDroppedCallback sampleFilesDroppedCallback = {},
                            WaveformPreviewRequestCallback waveformPreviewRequestCallback = {},
                            SourceValidationStatusProvider sourceValidationStatusProvider = {},
                            DraftPlaybackActionCallback onRequestSourceValidation = {},
                            DraftPlaybackActionCallback onCancelSourceValidation = {});
    ~AuthoringPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void reloadFromSession();
    void refreshNow();
    // Shells can forward a learned MIDI note here while the Articulations drawer
    // is listening. The value is validated through the normal project transaction.
    bool applyLearnedKeySwitchMidiNote(int midiNote);

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
    void rebuildGroupList();
    void rebuildMacroList();
    void rebuildFxSelector();
    void rebuildDspScopeSelector();
    void rebuildRoutingBusSelector();
    void rebuildPerformanceBankSelector();
    void rebuildTriggerSlotSelector();
    void rebuildArticulationList();
    void refreshInspectorVisibility();
    void refreshDrawerContextLabels();
    void refreshContextualAccessibility();
    void refreshWaveformDrawerContent();
    void updateSourceValidationAction();
    void requestWaveformPreviewLoad(bool refreshImmediately = false);
    void refreshDraftPlaybackBanner();
    void refreshFromSession();
    void applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values, const juce::String& label);
    void applySelectedGroupNameEdit();
    void applySelectedGroupMixEdit(const juce::String& label);
    void createGroup();
    void assignSelectedZonesToSelectedGroup();
    void deleteSelectedGroup();
    void moveSelectedGroup(int direction);
    void toggleSelectedGroupVisibility();
    void previewSelectedGroupAnchor();
    void applySelectedMacroEdit(const juce::String& label);
    void createMacro();
    void duplicateSelectedMacro();
    void deleteSelectedMacro();
    void moveSelectedMacro(int direction);
    void applySelectedFxSlotEdit(const juce::String& label);
    void createScopedFxSlot();
    void duplicateSelectedFxSlot();
    void deleteSelectedFxSlot();
    void moveSelectedFxSlot(int direction);
    void moveSelectedFxSlotToSelectedOwner();
    void applySelectedFxParameterEdit(const juce::String& label);
    void resetSelectedFxParameter();
    void assignSelectedFxParameterToMacro();
    std::string selectedDspScopeInputSource() const;
    std::string selectedDspScopeRoutingBusId() const;
    std::string ensureSelectedDspScopeRoutingBus();
    void applySelectedRoutingBusEdit(const juce::String& label);
    void applySelectedTriggerSlotEdit(const juce::String& label);
    void createArticulation();
    void duplicateSelectedArticulation();
    void applySelectedArticulationEdit(const juce::String& label);
    void moveSelectedArticulation(int direction);
    void deleteSelectedArticulation();
    void setSelectedArticulationDefault();
    void clearSelectedArticulationKeySwitch();
    void toggleKeySwitchMidiLearn();
    void importPhraseForSelectedBank();
    void previewSelectedZone(
        drs::engine::AuthoringPreviewAuditionSource source
            = drs::engine::AuthoringPreviewAuditionSource::summaryPreview,
        int explicitMidiNote = -1,
        int explicitVelocity = 0,
        std::string explicitZoneId = {});
    void releaseTimedPreview(std::size_t sourceIndex);
    void prepareDraftPlaybackPreview();
    void publishDraftPlayback();
    void undoLastEdit();
    void redoLastEdit();
    void markSavedCheckpoint();
    std::vector<drs::engine::AuthoringZoneSummary> buildVisibleZoneSummaries() const;
    void syncZoneMapSelectionState();
    bool applyZoneMapSelectionState(const authoring::ZoneMapCanvas::SelectionState& selectionState);
    std::size_t getZoneMapSelectionCount() const;
    void setDrawerOpen(bool shouldOpen);
    void setActiveDrawerTab(authoring::DrawerTab nextTab);
    void configureAccessibilityAndFocus();
    void refreshDrawerVisibility();
    void timerCallback(int timerId) override;
    authoring::SelectionSummaryViewModel buildSelectionSummaryViewModel() const;
    authoring::ZoneFieldValuesViewModel buildZoneFieldValuesViewModel() const;
    std::vector<std::string> collectSelectedZoneIdsForGrouping() const;

    drs::engine::AuthoringSession& authoringSession;
    WaveformPreviewProvider waveformPreviewProvider;
    WaveformPreviewRequestCallback waveformPreviewRequestCallback;
    AuthoringPreviewStatusProvider authoringPreviewStatusProvider;
    ImportResponsivenessProvider importResponsivenessProvider;
    SourceValidationStatusProvider sourceValidationStatusProvider;
    LayoutMode layoutMode = LayoutMode::compact;
    RestoreRootKeyCallback onRestoreRootKeyRequested;
    DraftPlaybackStatusProvider draftPlaybackStatusProvider;
    DraftPlaybackActionCallback onPrepareDraftPlaybackRequested;
    DraftPlaybackActionCallback onPublishDraftPlaybackRequested;
    DraftPlaybackActionCallback onRequestSourceValidation;
    DraftPlaybackActionCallback onCancelSourceValidation;
    PreviewCommandCallback previewCommandCallback;
    SampleFilesDroppedCallback sampleFilesDroppedCallback;
    struct TimedPreviewNote
    {
        bool active = false;
        int midiNote = 60;
        double releaseAtMillis = 0.0;
    };
    std::array<TimedPreviewNote, 4> timedPreviewNotes {};
    bool isRefreshing = false;
    int selectedGroupIndex = 0;
    int selectedMacroIndex = 0;
    int selectedFxSlotIndex = 0;
    int selectedDspScopeIndex = 2;
    int selectedFxParameterIndex = 0;
    std::vector<std::string> scopedFxSlotIds;
    std::vector<std::string> fxOwnerBusIds;
    std::vector<std::string> fxParameterIds;
    int selectedRoutingBusIndex = 0;
    int selectedPerformanceBankIndex = 0;
    int selectedTriggerSlotIndex = 0;
    int selectedArticulationIndex = 0;
    bool keySwitchMidiLearnActive = false;
    double keySwitchMidiLearnDeadlineMillis = 0.0;
    authoring::DrawerState drawerState;
    authoring::SelectionSummaryViewModel selectionSummaryViewModel;
    authoring::ZoneFieldValuesViewModel zoneFieldValuesViewModel;
    std::vector<std::string> zoneMapSelectedZoneIds;
    AuthoringControlLookAndFeel authoringLookAndFeel;

    authoring::AuthoringSummaryStrip summaryStrip;
    juce::Component playbackBanner;
    juce::Label playbackBannerLabel;
    juce::TextButton playbackBannerPrepareButton;
    juce::TextButton playbackBannerPublishButton;
    juce::Label waveformLabel;
    juce::Label waveformScopeLabel;
    juce::Label drawerBreadcrumbLabel;
    juce::Label waveformStatusLabel;
    juce::Label waveformInfoLabel;
    juce::Label loopInfoLabel;
    juce::Label importMetricsLabel;
    juce::Label sourceValidationLabel;
    juce::TextButton sourceValidationButton;
    juce::Component drawerRegion;
    juce::Component drawerTabStrip;
    juce::Component drawerContentHost;
    juce::TextButton drawerToggleButton;
    juce::TextButton drawerWaveformTabButton;
    juce::TextButton drawerGroupsTabButton;
    juce::TextButton drawerMacrosTabButton;
    juce::TextButton drawerRoutingTabButton;
    juce::TextButton drawerPerformanceTabButton;
    juce::TextButton drawerArticulationsTabButton;
    juce::Label zoneLabel;
    juce::ComboBox zoneSelector;
    juce::ToggleButton previewEnabledToggle;
    juce::TextButton previewStopButton;
    authoring::ZoneMapCanvas zoneMap;
    authoring::ZoneMappingEditor zoneMappingEditor;
    authoring::WaveformDetailView waveformPreview;
    juce::Label groupSectionLabel;
    juce::Label groupNameLabel;
    juce::TextEditor groupNameEditor;
    juce::TextButton groupCreateButton;
    juce::TextButton groupAssignZonesButton;
    juce::TextButton groupPreviewAnchorButton;
    authoring::RepeatedStructureList groupList;
    juce::TextButton groupMoveUpButton;
    juce::TextButton groupMoveDownButton;
    juce::TextButton groupVisibilityButton;
    juce::Label groupVisibilityHintLabel;

    juce::Component macroDrawerContent;
    juce::Viewport macroDrawerViewport;
    authoring::RepeatedStructureList macroList;
    juce::TextButton macroCreateButton;
    juce::TextButton macroDuplicateButton;
    juce::TextButton macroDeleteButton;
    juce::Label macroNameLabel;
    juce::TextEditor macroNameEditor;
    juce::Label macroExposeLabel;
    juce::ToggleButton macroExposeToggle;
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

    juce::Component routingDrawerContent;
    juce::Viewport routingDrawerViewport;
    juce::Label fxSectionLabel;
    juce::Label fxScopeLabel;
    juce::ComboBox fxScopeSelector;
    juce::Label fxScopeBreadcrumbLabel;
    juce::ComboBox fxSelector;
    juce::TextEditor fxNameEditor;
    juce::Label fxTypeLabel;
    juce::ComboBox fxTypeSelector;
    juce::ToggleButton fxBypassedToggle;
    juce::TextButton fxAddButton;
    juce::TextButton fxDuplicateButton;
    juce::TextButton fxMoveUpButton;
    juce::TextButton fxMoveDownButton;
    juce::TextButton fxDeleteButton;
    juce::ComboBox fxOwnerSelector;
    juce::TextButton fxMoveOwnerButton;
    juce::ComboBox fxParameterSelector;
    juce::Slider fxParameterSlider;
    juce::TextButton fxParameterResetButton;
    juce::TextButton fxAssignMacroButton;
    juce::Label fxParameterValueLabel;
    juce::Label fxSummaryLabel;
    juce::Label fxDiagnosticsLabel;

    juce::Label routingSectionLabel;
    juce::ComboBox routingBusSelector;
    juce::Label routingInputLabel;
    juce::ComboBox routingInputSelector;
    juce::Label routingInsertOneLabel;
    juce::ComboBox routingInsertOneSelector;
    juce::Label routingInsertTwoLabel;
    juce::ComboBox routingInsertTwoSelector;
    juce::Label routingSummaryLabel;
    juce::Label groupSummaryLabel;
    juce::Label groupVisibilityLabel;
    juce::ToggleButton groupVisibilityToggle;
    juce::Label groupGainLabel;
    juce::Slider groupGainSlider;
    juce::Label groupPanLabel;
    juce::Slider groupPanSlider;
    juce::Label groupRoutingLabel;
    juce::ComboBox groupRoutingSelector;
    juce::Label groupAnchorLabel;
    juce::ComboBox groupAnchorSelector;
    juce::TextButton groupDeleteButton;
    juce::Label groupRoundRobinLabel;
    juce::Label groupRoundRobinHintLabel;
    juce::ToggleButton groupRoundRobinToggle;
    juce::ComboBox groupRoundRobinModeSelector;

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

    juce::Component articulationDrawerContent;
    juce::Viewport articulationDrawerViewport;
    authoring::RepeatedStructureList articulationList;
    juce::TextButton articulationCreateButton;
    juce::TextButton articulationDuplicateButton;
    juce::TextButton articulationDefaultButton;
    juce::TextButton articulationMoveUpButton;
    juce::TextButton articulationMoveDownButton;
    juce::TextButton articulationDeleteButton;
    juce::Label articulationNameLabel;
    juce::TextEditor articulationNameEditor;
    juce::Label articulationSwitchNoteLabel;
    juce::Slider articulationSwitchNoteSlider;
    juce::Label articulationSwitchNoteValueLabel;
    juce::TextButton articulationClearSwitchButton;
    juce::TextButton articulationMidiLearnButton;
    juce::Label articulationDeleteReassignLabel;
    juce::ComboBox articulationDeleteReassignSelector;
    juce::Label articulationStatusLabel;
    std::vector<std::unique_ptr<juce::TextButton>> articulationKeyButtons;
    std::vector<std::string> routingInputSourceIds;
    std::vector<std::string> groupRoutingBusIds;
    std::vector<std::string> groupAnchorZoneIds;

};
} // namespace drs::app
