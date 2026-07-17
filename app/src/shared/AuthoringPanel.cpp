#include "shared/AuthoringPanel.h"

#include "shared/authoring/AuthoringWorkspaceLayout.h"

#include <algorithm>
#include <array>

namespace drs::app
{
namespace
{
const auto authoringPanelBackground = juce::Colour::fromRGB(18, 24, 29);
const auto authoringPanelCard = juce::Colour::fromRGB(250, 247, 240);
const auto authoringPanelAccent = juce::Colour::fromRGB(181, 96, 21);
const auto authoringPanelMuted = juce::Colour::fromRGB(82, 86, 94);
const auto authoringPanelSelected = juce::Colour::fromRGB(28, 108, 88);
const auto authoringPanelGrid = juce::Colour::fromRGB(230, 220, 207);

struct CuratedMacroAssignment
{
    const char* parameterId;
    const char* parameterPath;
    const char* defaultRole;
    const char* label;
};

constexpr std::array<CuratedMacroAssignment, 4> curatedMacroAssignments
{
    CuratedMacroAssignment{"filter-cutoff", "engine.filter.main.cutoff", "timbre", "Filter cutoff"},
    CuratedMacroAssignment{"voice-pitch", "engine.pitch.main.semitones", "motion", "Voice pitch"},
    CuratedMacroAssignment{"zone-gain", "authoring.zone.gainDb", "mix", "Zone gain"},
    CuratedMacroAssignment{"zone-pan", "authoring.zone.pan", "placement", "Zone pan"}
};

constexpr std::array<const char*, 5> curatedMacroRoles
{
    "timbre",
    "motion",
    "mix",
    "space",
    "placement"
};

constexpr std::array<const char*, 5> curatedFxTypes
{
    "eq",
    "delay",
    "reverb",
    "chorus",
    "saturator"
};

constexpr std::array<const char*, 3> curatedTriggerEvents
{
    "phrase-trigger",
    "key-switch",
    "phrase-latch"
};

constexpr std::array<const char*, 3> curatedChordModes
{
    "off",
    "follow-root",
    "preserve-intervals"
};

void configureEditorSlider(juce::Slider& slider,
                           double minValue,
                           double maxValue,
                           double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 24);
    slider.setRange(minValue, maxValue, interval);
}

void configureSectionLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
}

void configureFieldLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
}

juce::String formatZoneRange(const drs::engine::AuthoringZoneSummary& zone)
{
    return "Keys " + juce::String(zone.keyLow) + "-" + juce::String(zone.keyHigh)
        + " | Vel " + juce::String(zone.velocityLow) + "-" + juce::String(zone.velocityHigh);
}

juce::String formatMicros(std::uint64_t micros)
{
    if (micros >= 1000)
        return juce::String(static_cast<double>(micros) / 1000.0, 2) + " ms";

    return juce::String(static_cast<int>(micros)) + " us";
}

juce::String joinIdList(const std::vector<std::string>& values)
{
    if (values.empty())
        return "(none)";

    juce::String result;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
            result << " -> ";
        result << juce::String::fromUTF8(values[index].c_str());
    }

    return result;
}

std::vector<std::string> buildArticulationIds(const drs::engine::RuntimeProjectModel& project)
{
    std::vector<std::string> articulationIds;

    for (const auto& zone : project.authoring.zones)
    {
        if (std::find(articulationIds.begin(), articulationIds.end(), zone.articulationId) == articulationIds.end())
            articulationIds.push_back(zone.articulationId);
    }

    return articulationIds;
}

int findAssignmentIndex(const std::string& parameterId)
{
    for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
    {
        if (parameterId == curatedMacroAssignments[index].parameterId)
            return static_cast<int>(index);
    }

    return -1;
}

bool isExpandedLayout(AuthoringPanel::LayoutMode layoutMode)
{
    return layoutMode == AuthoringPanel::LayoutMode::expanded;
}

const char* getDrawerTabName(authoring::DrawerTab tab)
{
    switch (tab)
    {
        case authoring::DrawerTab::waveform:
            return "Waveform";
        case authoring::DrawerTab::macros:
            return "Macros";
        case authoring::DrawerTab::routing:
            return "Routing";
        case authoring::DrawerTab::performance:
            return "Performance";
        default:
            return "Drawer";
    }
}
} // namespace

AuthoringPanel::AuthoringPanel(drs::engine::AuthoringSession& session,
                               WaveformPreviewProvider previewProvider,
                               ImportResponsivenessProvider responsivenessProvider,
                               LayoutMode nextLayoutMode,
                               NotePreviewStartedCallback notePreviewStarted,
                               NotePreviewEndedCallback notePreviewEnded,
                               RestoreRootKeyCallback restoreRootKeyRequested)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      importResponsivenessProvider(std::move(responsivenessProvider)),
      layoutMode(nextLayoutMode),
      onNotePreviewStarted(std::move(notePreviewStarted)),
      onNotePreviewEnded(std::move(notePreviewEnded)),
      onRestoreRootKeyRequested(std::move(restoreRootKeyRequested))
{
    setComponentID("authoringWorkspace");
    drawerState.open = isExpandedLayout(layoutMode);
    drawerState.activeTab = authoring::DrawerTab::waveform;

    waveformInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    loopInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    importMetricsLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    drawerPlaceholderLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    macroSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    fxSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    routingSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    performanceSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    phraseSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);

    configureSectionLabel(waveformLabel, "Waveform Preview");
    configureSectionLabel(zoneLabel, "Selected Zone");
    configureSectionLabel(macroSectionLabel, "Macro Assignment");
    configureSectionLabel(fxSectionLabel, "Curated FX");
    configureSectionLabel(routingSectionLabel, "Routing");
    configureSectionLabel(performanceSectionLabel, "Performance Bank");

    configureFieldLabel(inspectorModeLabel, "Editor");
    configureFieldLabel(macroAssignmentLabel, "Parameter");
    configureFieldLabel(macroRoleLabel, "Role");
    configureFieldLabel(macroDefaultLabel, "Default");
    configureFieldLabel(macroMinLabel, "Min");
    configureFieldLabel(macroMaxLabel, "Max");
    configureFieldLabel(fxTypeLabel, "Type");
    configureFieldLabel(routingInputLabel, "Input Source");
    configureFieldLabel(routingInsertOneLabel, "Insert A");
    configureFieldLabel(routingInsertTwoLabel, "Insert B");
    configureFieldLabel(triggerEventLabel, "Trigger");
    configureFieldLabel(targetArticulationLabel, "Articulation");
    configureFieldLabel(phraseAssetLabel, "Phrase");
    configureFieldLabel(chordModeLabel, "Chord Rule");
    configureFieldLabel(phraseImportPathLabel, "MIDI Path");
    drawerPlaceholderLabel.setJustificationType(juce::Justification::centredLeft);

    configureEditorSlider(macroDefaultSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMinSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMaxSlider, 0.0, 1.0, 0.01);

    inspectorModeSelector.setComponentID("authoringModeSelector");
    inspectorModeSelector.addItem("Mapping", 1);
    inspectorModeSelector.addItem("Macros", 2);
    inspectorModeSelector.addItem("Routing", 3);
    inspectorModeSelector.addItem("Performance", 4);
    inspectorModeSelector.onChange = [this]
    {
        refreshInspectorVisibility();
        resized();
    };
    inspectorModeSelector.setSelectedId(1, juce::dontSendNotification);

    zoneSelector.setComponentID("authoringZoneSelector");
    zoneMap.setComponentID("authoringZoneMap");
    drawerRegion.setComponentID("authoringDrawer");
    drawerTabStrip.setComponentID("authoringDrawerTabStrip");
    drawerContentHost.setComponentID("authoringDrawerContentHost");
    drawerToggleButton.setComponentID("authoringDrawerToggleButton");
    drawerWaveformTabButton.setComponentID("authoringDrawerWaveformTab");
    drawerMacrosTabButton.setComponentID("authoringDrawerMacrosTab");
    drawerRoutingTabButton.setComponentID("authoringDrawerRoutingTab");
    drawerPerformanceTabButton.setComponentID("authoringDrawerPerformanceTab");
    drawerPlaceholderLabel.setComponentID("authoringDrawerPlaceholder");
    waveformPreview.setComponentID("authoringWaveformPreview");
    macroSelector.setComponentID("authoringMacroSelector");
    macroAssignmentSelector.setComponentID("authoringMacroAssignmentSelector");
    macroRoleSelector.setComponentID("authoringMacroRoleSelector");
    macroDefaultSlider.setComponentID("authoringMacroDefaultSlider");
    macroMinSlider.setComponentID("authoringMacroMinSlider");
    macroMaxSlider.setComponentID("authoringMacroMaxSlider");
    macroMoveUpButton.setComponentID("authoringMacroMoveUpButton");
    macroMoveDownButton.setComponentID("authoringMacroMoveDownButton");
    fxSelector.setComponentID("authoringFxSelector");
    fxTypeSelector.setComponentID("authoringFxTypeSelector");
    fxBypassedToggle.setComponentID("authoringFxBypassedToggle");
    routingBusSelector.setComponentID("authoringRoutingSelector");
    routingInputSelector.setComponentID("authoringRoutingInputSelector");
    routingInsertOneSelector.setComponentID("authoringRoutingInsertOneSelector");
    routingInsertTwoSelector.setComponentID("authoringRoutingInsertTwoSelector");
    performanceBankSelector.setComponentID("authoringPerformanceBankSelector");
    triggerSlotSelector.setComponentID("authoringTriggerSlotSelector");
    triggerEventSelector.setComponentID("authoringTriggerEventSelector");
    targetArticulationSelector.setComponentID("authoringTargetArticulationSelector");
    phraseAssetSelector.setComponentID("authoringPhraseAssetSelector");
    chordModeSelector.setComponentID("authoringChordModeSelector");
    phraseImportPathEditor.setComponentID("authoringPhraseImportPath");

    drawerToggleButton.onClick = [this]
    {
        setDrawerOpen(!drawerState.open);
    };
    drawerWaveformTabButton.setButtonText("Waveform");
    drawerMacrosTabButton.setButtonText("Macros");
    drawerRoutingTabButton.setButtonText("Routing");
    drawerPerformanceTabButton.setButtonText("Performance");
    drawerWaveformTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::waveform); };
    drawerMacrosTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::macros); };
    drawerRoutingTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::routing); };
    drawerPerformanceTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::performance); };

    authoring::SelectionSummaryCallbacks summaryCallbacks;
    summaryCallbacks.onPreviewRequested = [this] { previewSelectedZone(); };
    summaryCallbacks.onUndoRequested = [this] { undoLastEdit(); };
    summaryCallbacks.onRedoRequested = [this] { redoLastEdit(); };
    summaryCallbacks.onMarkSavedRequested = [this] { markSavedCheckpoint(); };
    summaryStrip.setCallbacks(std::move(summaryCallbacks));

    authoring::ZoneFieldCallbacks zoneCallbacks;
    zoneCallbacks.onCommitRequested = [this](const authoring::ZoneFieldValuesViewModel& values,
                                             const std::string& label)
    {
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    };
    zoneCallbacks.onRestoreRootKeyRequested = [this]
    {
        if (onRestoreRootKeyRequested)
            onRestoreRootKeyRequested();
    };
    zoneMappingEditor.setCallbacks(std::move(zoneCallbacks));
    zoneMap.setOnZoneSelectionRequested([this](const std::string& zoneId)
    {
        if (isRefreshing)
            return;

        const auto selectedZone = authoringSession.getSelectedZone();
        if (selectedZone.has_value() && selectedZone->id == zoneId)
            return;

        authoringSession.selectZone(zoneId);
        refreshFromSession();
    });
    zoneMap.setOnZoneRangeCommitRequested([this](const drs::engine::AuthoringZoneSummary& zone,
                                                 const std::string& label)
    {
        authoring::ZoneFieldValuesViewModel values;
        values.hasSelection = true;
        values.rootKey = zone.rootKey;
        values.keyLow = zone.keyLow;
        values.keyHigh = zone.keyHigh;
        values.velocityLow = zone.velocityLow;
        values.velocityHigh = zone.velocityHigh;
        values.gainDb = zone.gainDb;
        values.pan = zone.pan;
        values.loopEnabled = zone.loopEnabled;
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    });

    zoneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        const auto zoneIndex = zoneSelector.getSelectedId() - 1;
        const auto zones = authoringSession.getZoneSummaries();
        if (zoneIndex < 0 || static_cast<std::size_t>(zoneIndex) >= zones.size())
            return;

        authoringSession.selectZone(zones[static_cast<std::size_t>(zoneIndex)].id);
        refreshFromSession();
    };

    macroSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedMacroIndex = std::max(0, macroSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    fxSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedFxSlotIndex = std::max(0, fxSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    routingBusSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedRoutingBusIndex = std::max(0, routingBusSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    performanceBankSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedPerformanceBankIndex = std::max(0, performanceBankSelector.getSelectedId() - 1);
        const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
        if (selectedPerformanceBankIndex >= 0
            && static_cast<std::size_t>(selectedPerformanceBankIndex) < performanceBanks.size())
        {
            authoringSession.selectPerformanceBank(
                performanceBanks[static_cast<std::size_t>(selectedPerformanceBankIndex)].id);
        }

        selectedTriggerSlotIndex = 0;
        refreshFromSession();
    };

    triggerSlotSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedTriggerSlotIndex = std::max(0, triggerSlotSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    auto bindCommitOnDragEnd = [this](juce::Slider& slider, const juce::String& label, auto&& callback)
    {
        slider.onDragEnd = [this, label, callback]
        {
            callback(label);
        };
    };

    bindCommitOnDragEnd(macroDefaultSlider, "Update macro default", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMinSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMaxSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });

    macroAssignmentSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro assignment");
    };

    macroRoleSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro role");
    };

    macroMoveUpButton.setButtonText("Move Up");
    macroMoveUpButton.onClick = [this] { moveSelectedMacro(-1); };
    macroMoveDownButton.setButtonText("Move Down");
    macroMoveDownButton.onClick = [this] { moveSelectedMacro(1); };

    fxTypeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Update FX type");
    };

    fxBypassedToggle.setButtonText("Bypassed");
    fxBypassedToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Toggle FX bypass");
    };

    routingInputSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing input");
    };

    routingInsertOneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    routingInsertTwoSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    triggerEventSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger event");
    };

    targetArticulationSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger articulation");
    };

    phraseAssetSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger phrase asset");
    };

    chordModeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger chord rule");
    };

    phraseImportButton.setButtonText("Import MIDI Phrase");
    phraseImportButton.setComponentID("authoringPhraseImportButton");
    phraseImportButton.onClick = [this]
    {
        importPhraseForSelectedBank();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&summaryStrip),
             static_cast<juce::Component*>(&drawerRegion),
             static_cast<juce::Component*>(&drawerTabStrip),
             static_cast<juce::Component*>(&drawerContentHost),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&drawerToggleButton),
             static_cast<juce::Component*>(&drawerWaveformTabButton),
             static_cast<juce::Component*>(&drawerMacrosTabButton),
             static_cast<juce::Component*>(&drawerRoutingTabButton),
             static_cast<juce::Component*>(&drawerPerformanceTabButton),
             static_cast<juce::Component*>(&drawerPlaceholderLabel),
             static_cast<juce::Component*>(&inspectorModeLabel),
             static_cast<juce::Component*>(&inspectorModeSelector),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&zoneMap),
             static_cast<juce::Component*>(&zoneMappingEditor),
             static_cast<juce::Component*>(&waveformPreview),
             static_cast<juce::Component*>(&macroSectionLabel),
             static_cast<juce::Component*>(&macroSelector),
             static_cast<juce::Component*>(&macroAssignmentLabel),
             static_cast<juce::Component*>(&macroAssignmentSelector),
             static_cast<juce::Component*>(&macroRoleLabel),
             static_cast<juce::Component*>(&macroRoleSelector),
             static_cast<juce::Component*>(&macroDefaultLabel),
             static_cast<juce::Component*>(&macroDefaultSlider),
             static_cast<juce::Component*>(&macroMinLabel),
             static_cast<juce::Component*>(&macroMinSlider),
             static_cast<juce::Component*>(&macroMaxLabel),
             static_cast<juce::Component*>(&macroMaxSlider),
             static_cast<juce::Component*>(&macroSummaryLabel),
             static_cast<juce::Component*>(&macroMoveUpButton),
             static_cast<juce::Component*>(&macroMoveDownButton),
             static_cast<juce::Component*>(&fxSectionLabel),
             static_cast<juce::Component*>(&fxSelector),
             static_cast<juce::Component*>(&fxTypeLabel),
             static_cast<juce::Component*>(&fxTypeSelector),
             static_cast<juce::Component*>(&fxBypassedToggle),
             static_cast<juce::Component*>(&fxSummaryLabel),
             static_cast<juce::Component*>(&routingSectionLabel),
             static_cast<juce::Component*>(&routingBusSelector),
             static_cast<juce::Component*>(&routingInputLabel),
             static_cast<juce::Component*>(&routingInputSelector),
             static_cast<juce::Component*>(&routingInsertOneLabel),
             static_cast<juce::Component*>(&routingInsertOneSelector),
             static_cast<juce::Component*>(&routingInsertTwoLabel),
             static_cast<juce::Component*>(&routingInsertTwoSelector),
             static_cast<juce::Component*>(&routingSummaryLabel),
             static_cast<juce::Component*>(&performanceSectionLabel),
             static_cast<juce::Component*>(&performanceBankSelector),
             static_cast<juce::Component*>(&triggerSlotSelector),
             static_cast<juce::Component*>(&triggerEventLabel),
             static_cast<juce::Component*>(&triggerEventSelector),
             static_cast<juce::Component*>(&targetArticulationLabel),
             static_cast<juce::Component*>(&targetArticulationSelector),
             static_cast<juce::Component*>(&phraseAssetLabel),
             static_cast<juce::Component*>(&phraseAssetSelector),
             static_cast<juce::Component*>(&chordModeLabel),
             static_cast<juce::Component*>(&chordModeSelector),
             static_cast<juce::Component*>(&phraseImportPathLabel),
             static_cast<juce::Component*>(&phraseImportPathEditor),
             static_cast<juce::Component*>(&phraseImportButton),
             static_cast<juce::Component*>(&performanceSummaryLabel),
             static_cast<juce::Component*>(&phraseSummaryLabel)
         })
    {
        addAndMakeVisible(component);
    }

    refreshFromSession();
}

void AuthoringPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoringPanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(authoringPanelAccent.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds, 20.0f);

    g.setColour(authoringPanelCard);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);
}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);

    summaryStrip.setBounds(area.removeFromTop(authoring::heroHeight));

    area.removeFromTop(12);
    auto toolbarRow = area.removeFromTop(28);
    zoneLabel.setBounds(toolbarRow.removeFromLeft(96));
    toolbarRow.removeFromLeft(8);
    zoneSelector.setBounds(toolbarRow.removeFromLeft(250));
    toolbarRow.removeFromLeft(18);
    inspectorModeLabel.setBounds(toolbarRow.removeFromLeft(60));
    toolbarRow.removeFromLeft(8);
    inspectorModeSelector.setBounds(toolbarRow.removeFromLeft(210));

    area.removeFromTop(12);
    const auto expanded = isExpandedLayout(layoutMode);
    const auto drawerOpenHeight = expanded ? authoring::expandedDrawerOpenHeight
                                           : authoring::compactDrawerOpenHeight;
    const auto drawerHeight = authoring::drawerTabStripHeight + (drawerState.open ? drawerOpenHeight : 0);
    auto drawerArea = area.removeFromBottom(std::min(drawerHeight, area.getHeight()));
    drawerRegion.setBounds(drawerArea);
    drawerTabStrip.setBounds(drawerArea.removeFromTop(authoring::drawerTabStripHeight));
    drawerContentHost.setBounds(drawerArea);

    auto toggleArea = drawerTabStrip.getBounds().reduced(0, 4);
    drawerToggleButton.setBounds(toggleArea.removeFromRight(110));

    auto tabArea = drawerTabStrip.getBounds().reduced(0, 4);
    const auto tabWidth = expanded ? 104 : 96;
    drawerWaveformTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerMacrosTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerRoutingTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(8);
    drawerPerformanceTabButton.setBounds(tabArea.removeFromLeft(tabWidth + 10));

    auto drawerContent = drawerContentHost.getBounds().reduced(12, 10);
    waveformLabel.setBounds(drawerContent.removeFromTop(24));
    drawerContent.removeFromTop(6);
    waveformPreview.setBounds(drawerContent.removeFromTop(authoring::waveformPreviewHeight));
    drawerContent.removeFromTop(6);
    waveformInfoLabel.setBounds(drawerContent.removeFromTop(22));
    loopInfoLabel.setBounds(drawerContent.removeFromTop(22));
    importMetricsLabel.setBounds(drawerContent.removeFromTop(38));
    drawerPlaceholderLabel.setBounds(drawerContentHost.getBounds().reduced(16, 14));

    area.removeFromTop(8);
    auto shellArea = area;
    const auto desiredInspectorWidth = expanded ? authoring::expandedInspectorPreferredWidth
                                                : authoring::compactInspectorPreferredWidth;
    const auto minimumInspectorWidth = expanded ? authoring::expandedInspectorMinWidth
                                                : authoring::compactInspectorMinWidth;
    const auto maximumInspectorWidth = expanded ? authoring::expandedInspectorMaxWidth
                                                : authoring::compactInspectorMaxWidth;
    const auto inspectorWidth = juce::jlimit(minimumInspectorWidth,
                                             std::min(maximumInspectorWidth, std::max(minimumInspectorWidth, shellArea.getWidth() / 2)),
                                             desiredInspectorWidth);

    auto inspector = shellArea.removeFromRight(inspectorWidth);
    shellArea.removeFromRight(14);
    zoneMap.setBounds(shellArea);

    auto layoutLabelAndField = [](juce::Rectangle<int> row,
                                  juce::Label& label,
                                  juce::Component& field,
                                  int labelWidth)
    {
        label.setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(6);
        field.setBounds(row);
    };

    auto layoutDualLabelAndFieldRow = [&](juce::Rectangle<int> row,
                                          juce::Label& leftLabel,
                                          juce::Component& leftField,
                                          int leftLabelWidth,
                                          juce::Label& rightLabel,
                                          juce::Component& rightField,
                                          int rightLabelWidth)
    {
        auto left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        auto right = row;
        layoutLabelAndField(left, leftLabel, leftField, leftLabelWidth);
        layoutLabelAndField(right, rightLabel, rightField, rightLabelWidth);
    };

    if (inspectorModeSelector.getSelectedId() == 1)
    {
        zoneMappingEditor.setBounds(inspector);
    }
    else if (inspectorModeSelector.getSelectedId() == 2)
    {
        macroSectionLabel.setBounds(inspector.removeFromTop(24));
        inspector.removeFromTop(4);

        if (inspector.getWidth() < 420)
        {
            macroSelector.setBounds(inspector.removeFromTop(28));
            inspector.removeFromTop(4);

            auto buttonRow = inspector.removeFromTop(28);
            macroMoveUpButton.setBounds(buttonRow.removeFromLeft((buttonRow.getWidth() - 8) / 2));
            buttonRow.removeFromLeft(8);
            macroMoveDownButton.setBounds(buttonRow);
            inspector.removeFromTop(4);
        }
        else
        {
            auto selectorRow = inspector.removeFromTop(28);
            macroSelector.setBounds(selectorRow.removeFromLeft(260));
            selectorRow.removeFromLeft(8);
            macroMoveUpButton.setBounds(selectorRow.removeFromLeft(90));
            selectorRow.removeFromLeft(8);
            macroMoveDownButton.setBounds(selectorRow.removeFromLeft(90));
            inspector.removeFromTop(4);
        }
        auto row = inspector.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroAssignmentLabel,
                                   macroAssignmentSelector,
                                   76,
                                   macroRoleLabel,
                                   macroRoleSelector,
                                   56);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroDefaultLabel,
                                   macroDefaultSlider,
                                   56,
                                   macroMinLabel,
                                   macroMinSlider,
                                   40);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        layoutLabelAndField(row, macroMaxLabel, macroMaxSlider, 44);

        if (expanded)
        {
            inspector.removeFromTop(4);
            macroSummaryLabel.setBounds(inspector.removeFromTop(24));
        }
    }
    else if (inspectorModeSelector.getSelectedId() == 3)
    {
        auto headerRow = inspector.removeFromTop(24);
        auto leftHeader = headerRow.removeFromLeft((headerRow.getWidth() - 12) / 2);
        headerRow.removeFromLeft(12);
        fxSectionLabel.setBounds(leftHeader);
        routingSectionLabel.setBounds(headerRow);
        inspector.removeFromTop(4);

        auto row = inspector.removeFromTop(28);
        auto left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        auto right = row;
        fxSelector.setBounds(left);
        routingBusSelector.setBounds(right);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        right = row;
        layoutLabelAndField(left, fxTypeLabel, fxTypeSelector, 44);
        layoutLabelAndField(right, routingInputLabel, routingInputSelector, 44);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        right = row;
        fxBypassedToggle.setBounds(left);
        layoutLabelAndField(right, routingInsertOneLabel, routingInsertOneSelector, 44);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        layoutLabelAndField(row, routingInsertTwoLabel, routingInsertTwoSelector, 56);

        if (expanded)
        {
            inspector.removeFromTop(4);
            fxSummaryLabel.setBounds(inspector.removeFromTop(20));
            inspector.removeFromTop(2);
            routingSummaryLabel.setBounds(inspector.removeFromTop(20));
        }
    }
    else
    {
        performanceSectionLabel.setBounds(inspector.removeFromTop(24));
        inspector.removeFromTop(4);

        if (inspector.getWidth() < 420)
        {
            performanceBankSelector.setBounds(inspector.removeFromTop(28));
            inspector.removeFromTop(4);
            triggerSlotSelector.setBounds(inspector.removeFromTop(28));
            inspector.removeFromTop(4);
        }
        else
        {
            auto selectorRow = inspector.removeFromTop(28);
            performanceBankSelector.setBounds(selectorRow.removeFromLeft(250));
            selectorRow.removeFromLeft(10);
            triggerSlotSelector.setBounds(selectorRow.removeFromLeft(250));
            inspector.removeFromTop(4);
        }

        auto row = inspector.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   triggerEventLabel,
                                   triggerEventSelector,
                                   52,
                                   targetArticulationLabel,
                                   targetArticulationSelector,
                                   72);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   phraseAssetLabel,
                                   phraseAssetSelector,
                                   48,
                                   chordModeLabel,
                                   chordModeSelector,
                                   72);
        inspector.removeFromTop(4);

        row = inspector.removeFromTop(28);
        auto buttonArea = row.removeFromRight(180);
        row.removeFromRight(10);
        layoutLabelAndField(row, phraseImportPathLabel, phraseImportPathEditor, 56);
        phraseImportButton.setBounds(buttonArea);

        if (expanded)
        {
            inspector.removeFromTop(6);
            performanceSummaryLabel.setBounds(inspector.removeFromTop(20));
            inspector.removeFromTop(4);
            phraseSummaryLabel.setBounds(inspector.removeFromTop(24));
        }
    }
}

void AuthoringPanel::reloadFromSession()
{
    refreshFromSession();
}

authoring::SelectionSummaryViewModel AuthoringPanel::buildSelectionSummaryViewModel() const
{
    authoring::SelectionSummaryViewModel viewModel;
    const auto& documentState = authoringSession.getDocumentState();
    viewModel.title = "Phase 2 Authoring Workspace";
    viewModel.statusText = "Revision " + std::to_string(documentState.revision)
        + " | dirty=" + std::string(documentState.dirty ? "yes" : "no")
        + " | undo=" + std::to_string(documentState.undoDepth)
        + " | redo=" + std::to_string(documentState.redoDepth);
    viewModel.sourceText = "Sample source: none";
    viewModel.articulationText = "Articulation: none";
    viewModel.canUndo = documentState.undoDepth > 0;
    viewModel.canRedo = documentState.redoDepth > 0;
    viewModel.dirty = documentState.dirty;

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.sourceText = "Sample source: " + zone->sampleSourceId;
        viewModel.articulationText = "Articulation: " + zone->articulationId;
        viewModel.canPreview = true;
        viewModel.canRestoreRootKey = true;
    }

    return viewModel;
}

authoring::ZoneFieldValuesViewModel AuthoringPanel::buildZoneFieldValuesViewModel() const
{
    authoring::ZoneFieldValuesViewModel viewModel;
    viewModel.emptyStateText = "Select a zone to edit mapping values.";

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.hasSelection = true;
        viewModel.rootKey = zone->rootKey;
        viewModel.keyLow = zone->keyLow;
        viewModel.keyHigh = zone->keyHigh;
        viewModel.velocityLow = zone->velocityLow;
        viewModel.velocityHigh = zone->velocityHigh;
        viewModel.gainDb = zone->gainDb;
        viewModel.pan = zone->pan;
        viewModel.loopEnabled = zone->loopEnabled;
    }

    return viewModel;
}

void AuthoringPanel::rebuildZoneSelector()
{
    const auto zones = authoringSession.getZoneSummaries();
    zoneSelector.clear(juce::dontSendNotification);

    int itemId = 1;
    int selectedItemId = 0;
    for (const auto& zone : zones)
    {
        zoneSelector.addItem(juce::String::fromUTF8(zone.displayName.c_str())
                                 + "  ["
                                 + formatZoneRange(zone)
                                 + "]",
                             itemId);
        if (zone.selected)
            selectedItemId = itemId;
        ++itemId;
    }

    zoneSelector.setSelectedId(selectedItemId, juce::dontSendNotification);
}

void AuthoringPanel::rebuildMacroSelector()
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    macroSelector.clear(juce::dontSendNotification);

    if (macros.empty())
    {
        selectedMacroIndex = 0;
        return;
    }

    selectedMacroIndex = std::clamp(selectedMacroIndex, 0, static_cast<int>(macros.size()) - 1);
    for (std::size_t index = 0; index < macros.size(); ++index)
    {
        macroSelector.addItem(juce::String::fromUTF8(macros[index].name.c_str()),
                              static_cast<int>(index) + 1);
    }

    macroSelector.setSelectedId(selectedMacroIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildFxSelector()
{
    const auto& fxSlots = authoringSession.getProject().authoring.fxSlots;
    fxSelector.clear(juce::dontSendNotification);

    if (fxSlots.empty())
    {
        selectedFxSlotIndex = 0;
        return;
    }

    selectedFxSlotIndex = std::clamp(selectedFxSlotIndex, 0, static_cast<int>(fxSlots.size()) - 1);
    for (std::size_t index = 0; index < fxSlots.size(); ++index)
    {
        fxSelector.addItem(juce::String::fromUTF8(fxSlots[index].displayName.c_str()),
                           static_cast<int>(index) + 1);
    }

    fxSelector.setSelectedId(selectedFxSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildRoutingBusSelector()
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    routingBusSelector.clear(juce::dontSendNotification);

    if (routingBuses.empty())
    {
        selectedRoutingBusIndex = 0;
        return;
    }

    selectedRoutingBusIndex = std::clamp(selectedRoutingBusIndex, 0, static_cast<int>(routingBuses.size()) - 1);
    for (std::size_t index = 0; index < routingBuses.size(); ++index)
    {
        routingBusSelector.addItem(juce::String::fromUTF8(routingBuses[index].displayName.c_str()),
                                   static_cast<int>(index) + 1);
    }

    routingBusSelector.setSelectedId(selectedRoutingBusIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildPerformanceBankSelector()
{
    const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
    performanceBankSelector.clear(juce::dontSendNotification);

    if (performanceBanks.empty())
    {
        selectedPerformanceBankIndex = 0;
        return;
    }

    selectedPerformanceBankIndex = std::clamp(selectedPerformanceBankIndex,
                                              0,
                                              static_cast<int>(performanceBanks.size()) - 1);
    for (std::size_t index = 0; index < performanceBanks.size(); ++index)
    {
        performanceBankSelector.addItem(juce::String::fromUTF8(performanceBanks[index].displayName.c_str()),
                                        static_cast<int>(index) + 1);
    }

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        const auto iterator = std::find_if(performanceBanks.begin(),
                                           performanceBanks.end(),
                                           [&](const auto& performanceBank)
                                           {
                                               return performanceBank.id == selectedPerformanceBank->id;
                                           });
        if (iterator != performanceBanks.end())
            selectedPerformanceBankIndex = static_cast<int>(std::distance(performanceBanks.begin(), iterator));
    }

    performanceBankSelector.setSelectedId(selectedPerformanceBankIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildTriggerSlotSelector()
{
    triggerSlotSelector.clear(juce::dontSendNotification);

    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value() || selectedPerformanceBank->triggerSlots.empty())
    {
        selectedTriggerSlotIndex = 0;
        return;
    }

    selectedTriggerSlotIndex = std::clamp(selectedTriggerSlotIndex,
                                          0,
                                          static_cast<int>(selectedPerformanceBank->triggerSlots.size()) - 1);
    for (std::size_t index = 0; index < selectedPerformanceBank->triggerSlots.size(); ++index)
    {
        triggerSlotSelector.addItem(
            juce::String::fromUTF8(selectedPerformanceBank->triggerSlots[index].displayName.c_str()),
            static_cast<int>(index) + 1);
    }

    triggerSlotSelector.setSelectedId(selectedTriggerSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::setDrawerOpen(bool shouldOpen)
{
    if (drawerState.open == shouldOpen)
        return;

    drawerState.open = shouldOpen;
    refreshDrawerVisibility();
    resized();
}

void AuthoringPanel::setActiveDrawerTab(authoring::DrawerTab nextTab)
{
    drawerState.activeTab = nextTab;
    drawerState.open = true;
    refreshDrawerVisibility();
    resized();
}

void AuthoringPanel::refreshDrawerVisibility()
{
    const auto waveformTab = drawerState.activeTab == authoring::DrawerTab::waveform;
    const auto drawerContentVisible = drawerState.open;

    drawerToggleButton.setButtonText(drawerState.open ? "Hide Drawer" : "Show Drawer");
    drawerContentHost.setVisible(drawerContentVisible);
    waveformLabel.setVisible(drawerContentVisible && waveformTab);
    waveformPreview.setVisible(drawerContentVisible && waveformTab);
    waveformInfoLabel.setVisible(drawerContentVisible && waveformTab);
    loopInfoLabel.setVisible(drawerContentVisible && waveformTab);
    importMetricsLabel.setVisible(drawerContentVisible && waveformTab);
    drawerPlaceholderLabel.setVisible(drawerContentVisible && !waveformTab);
    drawerPlaceholderLabel.setText(
        juce::String::fromUTF8(getDrawerTabName(drawerState.activeTab))
            + " content remains behind the temporary editor selector during this Sprint 2 migration slice.",
        juce::dontSendNotification);

    drawerWaveformTabButton.setToggleState(waveformTab, juce::dontSendNotification);
    drawerMacrosTabButton.setToggleState(drawerState.activeTab == authoring::DrawerTab::macros,
                                         juce::dontSendNotification);
    drawerRoutingTabButton.setToggleState(drawerState.activeTab == authoring::DrawerTab::routing,
                                          juce::dontSendNotification);
    drawerPerformanceTabButton.setToggleState(drawerState.activeTab == authoring::DrawerTab::performance,
                                              juce::dontSendNotification);
}

void AuthoringPanel::refreshInspectorVisibility()
{
    const auto mappingMode = inspectorModeSelector.getSelectedId() == 1;
    const auto macroMode = inspectorModeSelector.getSelectedId() == 2;
    const auto routingMode = inspectorModeSelector.getSelectedId() == 3;
    const auto performanceMode = inspectorModeSelector.getSelectedId() == 4;
    const auto expanded = isExpandedLayout(layoutMode);

    zoneMap.setVisible(true);
    zoneMappingEditor.setVisible(mappingMode);

    macroSectionLabel.setVisible(macroMode);
    macroSelector.setVisible(macroMode);
    macroAssignmentLabel.setVisible(macroMode);
    macroAssignmentSelector.setVisible(macroMode);
    macroRoleLabel.setVisible(macroMode);
    macroRoleSelector.setVisible(macroMode);
    macroDefaultLabel.setVisible(macroMode);
    macroDefaultSlider.setVisible(macroMode);
    macroMinLabel.setVisible(macroMode);
    macroMinSlider.setVisible(macroMode);
    macroMaxLabel.setVisible(macroMode);
    macroMaxSlider.setVisible(macroMode);
    macroMoveUpButton.setVisible(macroMode);
    macroMoveDownButton.setVisible(macroMode);
    macroSummaryLabel.setVisible(macroMode && expanded);

    fxSectionLabel.setVisible(routingMode);
    fxSelector.setVisible(routingMode);
    fxTypeLabel.setVisible(routingMode);
    fxTypeSelector.setVisible(routingMode);
    fxBypassedToggle.setVisible(routingMode);
    fxSummaryLabel.setVisible(routingMode && expanded);
    routingSectionLabel.setVisible(routingMode);
    routingBusSelector.setVisible(routingMode);
    routingInputLabel.setVisible(routingMode);
    routingInputSelector.setVisible(routingMode);
    routingInsertOneLabel.setVisible(routingMode);
    routingInsertOneSelector.setVisible(routingMode);
    routingInsertTwoLabel.setVisible(routingMode);
    routingInsertTwoSelector.setVisible(routingMode);
    routingSummaryLabel.setVisible(routingMode && expanded);

    performanceSectionLabel.setVisible(performanceMode);
    performanceBankSelector.setVisible(performanceMode);
    triggerSlotSelector.setVisible(performanceMode);
    triggerEventLabel.setVisible(performanceMode);
    triggerEventSelector.setVisible(performanceMode);
    targetArticulationLabel.setVisible(performanceMode);
    targetArticulationSelector.setVisible(performanceMode);
    phraseAssetLabel.setVisible(performanceMode);
    phraseAssetSelector.setVisible(performanceMode);
    chordModeLabel.setVisible(performanceMode);
    chordModeSelector.setVisible(performanceMode);
    phraseImportPathLabel.setVisible(performanceMode);
    phraseImportPathEditor.setVisible(performanceMode);
    phraseImportButton.setVisible(performanceMode);
    performanceSummaryLabel.setVisible(performanceMode && expanded);
    phraseSummaryLabel.setVisible(performanceMode && expanded);

    refreshDrawerVisibility();
}

void AuthoringPanel::refreshFromSession()
{
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);

    rebuildZoneSelector();
    rebuildMacroSelector();
    rebuildFxSelector();
    rebuildRoutingBusSelector();
    rebuildPerformanceBankSelector();
    rebuildTriggerSlotSelector();
    zoneMap.setZoneSummaries(authoringSession.getZoneSummaries());

    const auto& project = authoringSession.getProject();
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    zoneFieldValuesViewModel = buildZoneFieldValuesViewModel();

    summaryStrip.setViewModel(selectionSummaryViewModel);
    zoneMappingEditor.setViewModel(zoneFieldValuesViewModel);

    if (!project.authoring.macros.empty())
    {
        const auto& macro = project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)];
        macroAssignmentSelector.clear(juce::dontSendNotification);
        int selectedAssignmentId = 0;
        for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
        {
            macroAssignmentSelector.addItem(curatedMacroAssignments[index].label, static_cast<int>(index) + 1);
        }

        if (!macro.targets.empty())
        {
            const auto assignmentIndex = findAssignmentIndex(macro.targets.front().parameterId);
            if (assignmentIndex >= 0)
            {
                selectedAssignmentId = assignmentIndex + 1;
            }
            else
            {
                const auto customItemId = static_cast<int>(curatedMacroAssignments.size()) + 1;
                macroAssignmentSelector.addItem("Custom: "
                                                   + juce::String::fromUTF8(macro.targets.front().parameterId.c_str()),
                                               customItemId);
                selectedAssignmentId = customItemId;
            }
        }
        macroAssignmentSelector.setSelectedId(selectedAssignmentId > 0 ? selectedAssignmentId : 1,
                                              juce::dontSendNotification);

        macroRoleSelector.clear(juce::dontSendNotification);
        int selectedRoleId = 0;
        const auto currentRole = !macro.targets.empty() ? macro.targets.front().role : std::string{};
        for (std::size_t index = 0; index < curatedMacroRoles.size(); ++index)
        {
            macroRoleSelector.addItem(curatedMacroRoles[index], static_cast<int>(index) + 1);
            if (currentRole == curatedMacroRoles[index])
                selectedRoleId = static_cast<int>(index) + 1;
        }
        if (selectedRoleId == 0 && !currentRole.empty())
        {
            const auto customRoleId = static_cast<int>(curatedMacroRoles.size()) + 1;
            macroRoleSelector.addItem("Custom: " + juce::String::fromUTF8(currentRole.c_str()), customRoleId);
            selectedRoleId = customRoleId;
        }
        macroRoleSelector.setSelectedId(selectedRoleId > 0 ? selectedRoleId : 1, juce::dontSendNotification);

        macroDefaultSlider.setRange(macro.minValue, macro.maxValue, 0.01);
        macroDefaultSlider.setValue(macro.defaultValue, juce::dontSendNotification);
        macroMinSlider.setValue(macro.minValue, juce::dontSendNotification);
        macroMaxSlider.setValue(macro.maxValue, juce::dontSendNotification);
        macroSummaryLabel.setText(
            "Target "
                + juce::String::fromUTF8(macro.targets.empty() ? "" : macro.targets.front().parameterPath.c_str())
                + " | range " + juce::String(macro.minValue, 2)
                + " to " + juce::String(macro.maxValue, 2),
            juce::dontSendNotification);
        macroMoveUpButton.setEnabled(selectedMacroIndex > 0);
        macroMoveDownButton.setEnabled(selectedMacroIndex + 1 < static_cast<int>(project.authoring.macros.size()));
    }
    else
    {
        macroSummaryLabel.setText("No macros are authored in this project yet.", juce::dontSendNotification);
        macroMoveUpButton.setEnabled(false);
        macroMoveDownButton.setEnabled(false);
    }

    if (!project.authoring.fxSlots.empty())
    {
        const auto& fxSlot = project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
        fxTypeSelector.clear(juce::dontSendNotification);
        int selectedFxTypeId = 0;
        for (std::size_t index = 0; index < curatedFxTypes.size(); ++index)
        {
            fxTypeSelector.addItem(curatedFxTypes[index], static_cast<int>(index) + 1);
            if (fxSlot.effectType == curatedFxTypes[index])
                selectedFxTypeId = static_cast<int>(index) + 1;
        }
        if (selectedFxTypeId == 0 && !fxSlot.effectType.empty())
        {
            const auto customTypeId = static_cast<int>(curatedFxTypes.size()) + 1;
            fxTypeSelector.addItem("Custom: " + juce::String::fromUTF8(fxSlot.effectType.c_str()), customTypeId);
            selectedFxTypeId = customTypeId;
        }
        fxTypeSelector.setSelectedId(selectedFxTypeId > 0 ? selectedFxTypeId : 1, juce::dontSendNotification);
        fxBypassedToggle.setToggleState(fxSlot.bypassed, juce::dontSendNotification);
        fxSummaryLabel.setText(
            juce::String::fromUTF8(fxSlot.id.c_str()) + " | "
                + (fxSlot.bypassed ? "bypassed" : "active"),
            juce::dontSendNotification);
    }
    else
    {
        fxSummaryLabel.setText("No FX slots are authored in this project yet.", juce::dontSendNotification);
    }

    if (!project.authoring.routingBuses.empty())
    {
        const auto& routingBus = project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
        std::vector<std::string> inputSources;
        inputSources.push_back("master");
        for (const auto& zone : project.authoring.zones)
            inputSources.push_back(zone.id);

        routingInputSelector.clear(juce::dontSendNotification);
        int selectedInputId = 0;
        for (std::size_t index = 0; index < inputSources.size(); ++index)
        {
            routingInputSelector.addItem(juce::String::fromUTF8(inputSources[index].c_str()),
                                         static_cast<int>(index) + 1);
            if (routingBus.inputSourceId == inputSources[index])
                selectedInputId = static_cast<int>(index) + 1;
        }
        routingInputSelector.setSelectedId(selectedInputId > 0 ? selectedInputId : 1, juce::dontSendNotification);

        auto refreshInsertSelector = [&](juce::ComboBox& combo, const std::string& selectedFxId)
        {
            combo.clear(juce::dontSendNotification);
            combo.addItem("(none)", 1);

            int selectedId = 1;
            for (std::size_t index = 0; index < project.authoring.fxSlots.size(); ++index)
            {
                const auto itemId = static_cast<int>(index) + 2;
                combo.addItem(juce::String::fromUTF8(project.authoring.fxSlots[index].id.c_str()), itemId);
                if (project.authoring.fxSlots[index].id == selectedFxId)
                    selectedId = itemId;
            }

            combo.setSelectedId(selectedId, juce::dontSendNotification);
        };

        refreshInsertSelector(routingInsertOneSelector,
                              routingBus.fxSlotIds.empty() ? std::string{} : routingBus.fxSlotIds.front());
        refreshInsertSelector(routingInsertTwoSelector,
                              routingBus.fxSlotIds.size() < 2 ? std::string{} : routingBus.fxSlotIds[1]);

        routingSummaryLabel.setText(
            "Bus " + juce::String::fromUTF8(routingBus.id.c_str())
                + " | chain " + joinIdList(routingBus.fxSlotIds),
            juce::dontSendNotification);
    }
    else
    {
        routingSummaryLabel.setText("No routing buses are authored in this project yet.", juce::dontSendNotification);
    }

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        triggerEventSelector.clear(juce::dontSendNotification);
        int selectedTriggerEventId = 0;
        for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
        {
            triggerEventSelector.addItem(curatedTriggerEvents[index], static_cast<int>(index) + 1);
        }

        targetArticulationSelector.clear(juce::dontSendNotification);
        const auto articulationIds = buildArticulationIds(project);
        int selectedArticulationId = 0;
        for (std::size_t index = 0; index < articulationIds.size(); ++index)
        {
            targetArticulationSelector.addItem(juce::String::fromUTF8(articulationIds[index].c_str()),
                                               static_cast<int>(index) + 1);
        }

        phraseAssetSelector.clear(juce::dontSendNotification);
        phraseAssetSelector.addItem("(none)", 1);
        int selectedPhraseAssetId = 1;
        for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
        {
            const auto itemId = static_cast<int>(index) + 2;
            phraseAssetSelector.addItem(juce::String::fromUTF8(selectedPerformanceBank->phraseAssets[index].displayName.c_str()),
                                        itemId);
        }

        chordModeSelector.clear(juce::dontSendNotification);
        int selectedChordModeId = 0;
        for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
        {
            chordModeSelector.addItem(curatedChordModes[index], static_cast<int>(index) + 1);
        }

        if (selectedTriggerSlotIndex >= 0
            && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size())
        {
            const auto& triggerSlot = selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
            for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
            {
                if (triggerSlot.triggerEvent == curatedTriggerEvents[index])
                    selectedTriggerEventId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < articulationIds.size(); ++index)
            {
                if (triggerSlot.targetArticulationId == articulationIds[index])
                    selectedArticulationId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
            {
                if (triggerSlot.phraseAssetId == selectedPerformanceBank->phraseAssets[index].id)
                    selectedPhraseAssetId = static_cast<int>(index) + 2;
            }

            for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
            {
                if (triggerSlot.chordMode == curatedChordModes[index])
                    selectedChordModeId = static_cast<int>(index) + 1;
            }

            performanceSummaryLabel.setText(
                "Trigger " + juce::String::fromUTF8(triggerSlot.displayName.c_str())
                    + " | event " + juce::String::fromUTF8(triggerSlot.triggerEvent.c_str())
                    + " | articulation " + juce::String::fromUTF8(triggerSlot.targetArticulationId.c_str()),
                juce::dontSendNotification);
        }
        else
        {
            performanceSummaryLabel.setText("No trigger slot is selected in the active performance bank.",
                                            juce::dontSendNotification);
        }

        triggerEventSelector.setSelectedId(selectedTriggerEventId > 0 ? selectedTriggerEventId : 1,
                                           juce::dontSendNotification);
        targetArticulationSelector.setSelectedId(selectedArticulationId > 0 ? selectedArticulationId : 1,
                                                 juce::dontSendNotification);
        phraseAssetSelector.setSelectedId(selectedPhraseAssetId, juce::dontSendNotification);
        chordModeSelector.setSelectedId(selectedChordModeId > 0 ? selectedChordModeId : 1,
                                        juce::dontSendNotification);

        if (!selectedPerformanceBank->phraseAssets.empty())
        {
            const auto phraseAssetIndex = std::max(0, phraseAssetSelector.getSelectedId() - 2);
            if (phraseAssetSelector.getSelectedId() > 1
                && static_cast<std::size_t>(phraseAssetIndex) < selectedPerformanceBank->phraseAssets.size())
            {
                const auto& phraseAsset = selectedPerformanceBank->phraseAssets[static_cast<std::size_t>(phraseAssetIndex)];
                phraseSummaryLabel.setText(
                    juce::String::fromUTF8(phraseAsset.displayName.c_str())
                        + " | notes=" + juce::String(static_cast<int>(phraseAsset.notes.size()))
                        + " | beats=" + juce::String(phraseAsset.lengthBeats, 2)
                        + " | chord=" + juce::String::fromUTF8(phraseAsset.chordHint.c_str()),
                    juce::dontSendNotification);
            }
            else
            {
                phraseSummaryLabel.setText("Phrase library ready. Select a phrase asset to inspect it.",
                                           juce::dontSendNotification);
            }
        }
        else
        {
            phraseSummaryLabel.setText("No MIDI phrases have been imported for the active performance bank yet.",
                                       juce::dontSendNotification);
        }
    }
    else
    {
        performanceSummaryLabel.setText("No performance bank is selected.", juce::dontSendNotification);
        phraseSummaryLabel.setText("Performance phrases unavailable.", juce::dontSendNotification);
    }

    if (waveformPreviewProvider)
    {
        const auto preview = waveformPreviewProvider();
        waveformPreview.setPreview(preview);
        waveformInfoLabel.setText(
            preview.available
                ? "Source " + juce::String::fromUTF8(preview.formatName.c_str())
                    + " | " + juce::String(static_cast<int>(preview.sampleRate)) + " Hz"
                    + " | " + juce::String(static_cast<int>(preview.channelCount)) + " ch"
                    + " | " + juce::String(preview.durationSeconds, 3) + " s"
                : "Waveform: " + juce::String::fromUTF8(preview.state.c_str()),
            juce::dontSendNotification);
        loopInfoLabel.setText(
            preview.available
                ? (preview.loopEnabled
                       ? "Loop " + juce::String(static_cast<int>(preview.loopStartFrame))
                           + " - " + juce::String(static_cast<int>(preview.loopEndFrame))
                       : "Loop disabled for selected zone")
                : "Loop metadata unavailable",
            juce::dontSendNotification);
    }

    if (importResponsivenessProvider)
    {
        const auto metrics = importResponsivenessProvider();
        importMetricsLabel.setText(
            metrics.available
                ? "Import responsiveness: items=" + juce::String(static_cast<int>(metrics.totalItemCount))
                    + ", processed=" + juce::String(static_cast<int>(metrics.processedCount))
                    + ", warnings=" + juce::String(static_cast<int>(metrics.warningItemCount))
                    + ", failures=" + juce::String(static_cast<int>(metrics.failedItemCount))
                    + " | last=" + formatMicros(metrics.lastProcessDurationMicros)
                    + ", avg=" + formatMicros(metrics.averageProcessDurationMicros)
                    + ", max=" + formatMicros(metrics.maxProcessDurationMicros)
                : "Import responsiveness unavailable",
            juce::dontSendNotification);
    }

    refreshInspectorVisibility();
}

void AuthoringPanel::applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values,
                                           const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    auto editedZone = *currentZone;
    editedZone.rootKey = values.rootKey;
    editedZone.keyLow = values.keyLow;
    editedZone.keyHigh = values.keyHigh;
    editedZone.velocityLow = values.velocityLow;
    editedZone.velocityHigh = values.velocityHigh;
    editedZone.gainDb = values.gainDb;
    editedZone.pan = values.pan;
    editedZone.loopEnabled = values.loopEnabled;

    authoringSession.updateSelectedZone(editedZone, label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::previewSelectedZone()
{
    const auto request = authoringSession.buildSelectedZonePreviewRequest();
    if (!request.available)
        return;

    if (onNotePreviewStarted)
        onNotePreviewStarted(request.midiNote, static_cast<float>(request.velocity) / 127.0f);

    if (onNotePreviewEnded)
    {
        juce::Timer::callAfterDelay(180,
                                    [callback = onNotePreviewEnded, midiNote = request.midiNote]()
                                    {
                                        callback(midiNote);
                                    });
    }
}

void AuthoringPanel::undoLastEdit()
{
    authoringSession.undo();
    refreshFromSession();
}

void AuthoringPanel::redoLastEdit()
{
    authoringSession.redo();
    refreshFromSession();
}

void AuthoringPanel::markSavedCheckpoint()
{
    authoringSession.markSaved();
    refreshFromSession();
}

void AuthoringPanel::applySelectedMacroEdit(const juce::String& label)
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0 || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    auto minValue = macroMinSlider.getValue();
    auto maxValue = macroMaxSlider.getValue();
    if (minValue > maxValue)
        std::swap(minValue, maxValue);

    editedMacro.minValue = minValue;
    editedMacro.maxValue = maxValue;
    editedMacro.defaultValue = std::clamp(macroDefaultSlider.getValue(), editedMacro.minValue, editedMacro.maxValue);

    if (editedMacro.targets.empty())
        editedMacro.targets.push_back({});

    const auto assignmentId = macroAssignmentSelector.getSelectedId();
    if (assignmentId > 0 && assignmentId <= static_cast<int>(curatedMacroAssignments.size()))
    {
        const auto& assignment = curatedMacroAssignments[static_cast<std::size_t>(assignmentId - 1)];
        editedMacro.targets.front().parameterId = assignment.parameterId;
        editedMacro.targets.front().parameterPath = assignment.parameterPath;
        if (editedMacro.targets.front().role.empty())
            editedMacro.targets.front().role = assignment.defaultRole;
    }

    auto selectedRoleText = macroRoleSelector.getText().toStdString();
    if (selectedRoleText.rfind("Custom: ", 0) == 0)
        selectedRoleText = selectedRoleText.substr(8);
    editedMacro.targets.front().role = selectedRoleText;

    authoringSession.updateMacro(static_cast<std::size_t>(selectedMacroIndex),
                                 editedMacro,
                                 label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::moveSelectedMacro(int direction)
{
    const auto result = authoringSession.moveMacro(static_cast<std::size_t>(selectedMacroIndex),
                                                   direction,
                                                   direction < 0 ? "Move macro earlier" : "Move macro later");
    if (result.applied)
        selectedMacroIndex = std::max(0, selectedMacroIndex + direction);

    refreshFromSession();
}

void AuthoringPanel::applySelectedFxSlotEdit(const juce::String& label)
{
    const auto& fxSlots = authoringSession.getProject().authoring.fxSlots;
    if (selectedFxSlotIndex < 0 || static_cast<std::size_t>(selectedFxSlotIndex) >= fxSlots.size())
        return;

    auto editedFxSlot = fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
    auto effectType = fxTypeSelector.getText().toStdString();
    if (effectType.rfind("Custom: ", 0) == 0)
        effectType = effectType.substr(8);
    editedFxSlot.effectType = effectType;
    editedFxSlot.bypassed = fxBypassedToggle.getToggleState();

    authoringSession.updateFxSlot(static_cast<std::size_t>(selectedFxSlotIndex),
                                  editedFxSlot,
                                  label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedRoutingBusEdit(const juce::String& label)
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    if (selectedRoutingBusIndex < 0
        || static_cast<std::size_t>(selectedRoutingBusIndex) >= routingBuses.size())
    {
        return;
    }

    auto editedRoutingBus = routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
    editedRoutingBus.inputSourceId = routingInputSelector.getText().toStdString();
    editedRoutingBus.fxSlotIds.clear();

    auto appendFxId = [&](const juce::ComboBox& selector)
    {
        const auto fxId = selector.getText().toStdString();
        if (fxId.empty() || fxId == "(none)")
            return;

        if (std::find(editedRoutingBus.fxSlotIds.begin(), editedRoutingBus.fxSlotIds.end(), fxId)
            == editedRoutingBus.fxSlotIds.end())
        {
            editedRoutingBus.fxSlotIds.push_back(fxId);
        }
    };

    appendFxId(routingInsertOneSelector);
    appendFxId(routingInsertTwoSelector);

    authoringSession.updateRoutingBus(static_cast<std::size_t>(selectedRoutingBusIndex),
                                      editedRoutingBus,
                                      label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedTriggerSlotEdit(const juce::String& label)
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value()
        || selectedTriggerSlotIndex < 0
        || static_cast<std::size_t>(selectedTriggerSlotIndex) >= selectedPerformanceBank->triggerSlots.size())
    {
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
    triggerSlot.triggerEvent = triggerEventSelector.getText().toStdString();
    triggerSlot.targetArticulationId = targetArticulationSelector.getText().toStdString();
    triggerSlot.phraseAssetId = phraseAssetSelector.getSelectedId() > 1 ? phraseAssetSelector.getText().toStdString() : std::string{};
    triggerSlot.chordMode = chordModeSelector.getText().toStdString();

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::importPhraseForSelectedBank()
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value())
        return;

    const auto midiPath = phraseImportPathEditor.getText().trim().toStdString();
    if (midiPath.empty())
    {
        phraseSummaryLabel.setText("Choose a MIDI file path before importing a phrase.",
                                   juce::dontSendNotification);
        return;
    }

    const juce::File midiFile(midiPath);
    const auto phraseId = midiFile.getFileNameWithoutExtension().replaceCharacters(" ", "-").toLowerCase().toStdString();
    const auto importResult = importMidiPhraseAsset(midiPath,
                                                    phraseId,
                                                    midiFile.getFileNameWithoutExtension().toStdString());
    if (!importResult.imported)
    {
        phraseSummaryLabel.setText("Import failed: "
                                       + juce::String::fromUTF8(importResult.issues.empty()
                                                                    ? importResult.state.c_str()
                                                                    : importResult.issues.front().c_str()),
                                   juce::dontSendNotification);
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    const auto existingPhraseIterator = std::find_if(
        editedPerformanceBank.phraseAssets.begin(),
        editedPerformanceBank.phraseAssets.end(),
        [&](const auto& phraseAsset)
        {
            return phraseAsset.id == importResult.phraseAsset.id;
        });

    if (existingPhraseIterator == editedPerformanceBank.phraseAssets.end())
        editedPerformanceBank.phraseAssets.push_back(importResult.phraseAsset);
    else
        *existingPhraseIterator = importResult.phraseAsset;

    if (selectedTriggerSlotIndex >= 0
        && static_cast<std::size_t>(selectedTriggerSlotIndex) < editedPerformanceBank.triggerSlots.size())
    {
        auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
        if (triggerSlot.triggerEvent == "phrase-trigger")
        {
            triggerSlot.phraseAssetId = importResult.phraseAsset.id;
            if (triggerSlot.chordMode.empty())
                triggerSlot.chordMode = "follow-root";
        }
    }

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           "Import performance phrase");
    phraseSummaryLabel.setText(
        juce::String::fromUTF8(importResult.phraseAsset.displayName.c_str())
            + " imported | notes=" + juce::String(static_cast<int>(importResult.phraseAsset.notes.size()))
            + " | chord=" + juce::String::fromUTF8(importResult.phraseAsset.chordHint.c_str()),
        juce::dontSendNotification);
    refreshFromSession();
}
} // namespace drs::app
