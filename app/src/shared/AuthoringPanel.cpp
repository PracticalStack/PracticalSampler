#include "shared/AuthoringPanel.h"

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
} // namespace

void AuthoringPanel::ZoneMapComponent::setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries)
{
    zoneSummaries = std::move(summaries);
    repaint();
}

void AuthoringPanel::ZoneMapComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::transparentBlack);
    g.setColour(authoringPanelGrid);
    g.fillRoundedRectangle(bounds, 14.0f);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 24));

    for (int key = 0; key <= 8; ++key)
    {
        const auto x = inner.getX() + (inner.getWidth() * static_cast<float>(key) / 8.0f);
        g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
    }

    for (int velocity = 0; velocity <= 4; ++velocity)
    {
        const auto y = inner.getY() + (inner.getHeight() * static_cast<float>(velocity) / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), inner.getX(), inner.getRight());
    }

    for (const auto& zone : zoneSummaries)
    {
        const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
        const auto width = std::max(10.0f,
                                    inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
        const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
        const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
        const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
        const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));

        const juce::Rectangle<float> zoneBounds(x, y, width, height);
        g.setColour(zone.selected ? authoringPanelSelected : authoringPanelAccent.withMultipliedAlpha(0.72f));
        g.fillRoundedRectangle(zoneBounds, 8.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(zone.displayName.c_str()),
                         zoneBounds.toNearestInt().reduced(6, 4),
                         juce::Justification::centredLeft,
                         1);
    }
}

AuthoringPanel::AuthoringPanel(drs::engine::AuthoringSession& session,
                               WaveformPreviewProvider previewProvider,
                               ImportResponsivenessProvider responsivenessProvider,
                               LayoutMode nextLayoutMode,
                               NotePreviewStartedCallback notePreviewStarted,
                               NotePreviewEndedCallback notePreviewEnded)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      importResponsivenessProvider(std::move(responsivenessProvider)),
      layoutMode(nextLayoutMode),
      onNotePreviewStarted(std::move(notePreviewStarted)),
      onNotePreviewEnded(std::move(notePreviewEnded))
{
    titleLabel.setText("Phase 2 Authoring Workspace", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    statusLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    sourceLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    articulationLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    waveformInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    loopInfoLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    importMetricsLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
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
    configureFieldLabel(rootKeyLabel, "Root Key");
    configureFieldLabel(keyLowLabel, "Key Low");
    configureFieldLabel(keyHighLabel, "Key High");
    configureFieldLabel(velocityLowLabel, "Velocity Low");
    configureFieldLabel(velocityHighLabel, "Velocity High");
    configureFieldLabel(gainLabel, "Gain (dB)");
    configureFieldLabel(panLabel, "Pan");
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

    configureEditorSlider(rootKeySlider, 0, 127, 1);
    configureEditorSlider(keyLowSlider, 0, 127, 1);
    configureEditorSlider(keyHighSlider, 0, 127, 1);
    configureEditorSlider(velocityLowSlider, 1, 127, 1);
    configureEditorSlider(velocityHighSlider, 1, 127, 1);
    configureEditorSlider(gainSlider, -24.0, 12.0, 0.1);
    configureEditorSlider(panSlider, -1.0, 1.0, 0.01);
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
    waveformPreview.setComponentID("authoringWaveformPreview");
    macroSelector.setComponentID("authoringMacroSelector");
    fxSelector.setComponentID("authoringFxSelector");
    routingBusSelector.setComponentID("authoringRoutingSelector");
    performanceBankSelector.setComponentID("authoringPerformanceBankSelector");
    triggerSlotSelector.setComponentID("authoringTriggerSlotSelector");
    phraseAssetSelector.setComponentID("authoringPhraseAssetSelector");
    phraseImportPathEditor.setComponentID("authoringPhraseImportPath");
    previewButton.setComponentID("authoringPreviewButton");
    undoButton.setComponentID("authoringUndoButton");
    redoButton.setComponentID("authoringRedoButton");
    saveCheckpointButton.setComponentID("authoringSaveButton");

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

    bindCommitOnDragEnd(rootKeySlider, "Update zone root key", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(keyLowSlider, "Update zone key range", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(keyHighSlider, "Update zone key range", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(velocityLowSlider, "Update zone velocity range", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(velocityHighSlider, "Update zone velocity range", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(gainSlider, "Update zone gain", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(panSlider, "Update zone pan", [this](const juce::String& label) { applySelectedZoneEdit(label); });
    bindCommitOnDragEnd(macroDefaultSlider, "Update macro default", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMinSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMaxSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });

    loopEnabledToggle.setButtonText("Loop Enabled");
    loopEnabledToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedZoneEdit("Toggle zone loop");
    };

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

    previewButton.setButtonText("Preview Selected Zone");
    previewButton.onClick = [this]
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
    };

    undoButton.setButtonText("Undo");
    undoButton.onClick = [this]
    {
        authoringSession.undo();
        refreshFromSession();
    };

    redoButton.setButtonText("Redo");
    redoButton.onClick = [this]
    {
        authoringSession.redo();
        refreshFromSession();
    };

    saveCheckpointButton.setButtonText("Mark Saved");
    saveCheckpointButton.onClick = [this]
    {
        authoringSession.markSaved();
        refreshFromSession();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&titleLabel),
             static_cast<juce::Component*>(&statusLabel),
             static_cast<juce::Component*>(&sourceLabel),
             static_cast<juce::Component*>(&articulationLabel),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&inspectorModeLabel),
             static_cast<juce::Component*>(&inspectorModeSelector),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&zoneMap),
             static_cast<juce::Component*>(&waveformPreview),
             static_cast<juce::Component*>(&rootKeySlider),
             static_cast<juce::Component*>(&keyLowSlider),
             static_cast<juce::Component*>(&keyHighSlider),
             static_cast<juce::Component*>(&velocityLowSlider),
             static_cast<juce::Component*>(&velocityHighSlider),
             static_cast<juce::Component*>(&gainSlider),
             static_cast<juce::Component*>(&panSlider),
             static_cast<juce::Component*>(&rootKeyLabel),
             static_cast<juce::Component*>(&keyLowLabel),
             static_cast<juce::Component*>(&keyHighLabel),
             static_cast<juce::Component*>(&velocityLowLabel),
             static_cast<juce::Component*>(&velocityHighLabel),
             static_cast<juce::Component*>(&gainLabel),
             static_cast<juce::Component*>(&panLabel),
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
             static_cast<juce::Component*>(&phraseSummaryLabel),
             static_cast<juce::Component*>(&loopEnabledToggle),
             static_cast<juce::Component*>(&previewButton),
             static_cast<juce::Component*>(&undoButton),
             static_cast<juce::Component*>(&redoButton),
             static_cast<juce::Component*>(&saveCheckpointButton)
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

void AuthoringPanel::WaveformPreviewComponent::setPreview(AuthoringWaveformPreview nextPreview)
{
    preview = std::move(nextPreview);
    repaint();
}

void AuthoringPanel::WaveformPreviewComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(authoringPanelGrid);
    g.fillRoundedRectangle(bounds, 14.0f);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 42));
    g.drawHorizontalLine(static_cast<int>(inner.getCentreY()), inner.getX(), inner.getRight());

    if (!preview.available || preview.points.empty())
    {
        g.setColour(authoringPanelMuted);
        g.drawFittedText(preview.state.empty() ? "Waveform unavailable" : juce::String::fromUTF8(preview.state.c_str()),
                         getLocalBounds().reduced(12),
                         juce::Justification::centred,
                         2);
        return;
    }

    juce::Path waveformPath;
    const auto widthPerPoint = inner.getWidth() / static_cast<float>(preview.points.size());
    for (std::size_t index = 0; index < preview.points.size(); ++index)
    {
        const auto x = inner.getX() + (static_cast<float>(index) + 0.5f) * widthPerPoint;
        const auto minY = juce::jmap(preview.points[index].minValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        const auto maxY = juce::jmap(preview.points[index].maxValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        waveformPath.startNewSubPath(x, minY);
        waveformPath.lineTo(x, maxY);
    }

    g.setColour(authoringPanelSelected);
    g.strokePath(waveformPath, juce::PathStrokeType(1.3f));

    if (preview.loopEnabled && preview.frameCount > 0)
    {
        const auto startX = inner.getX() + inner.getWidth()
            * (static_cast<float>(preview.loopStartFrame) / static_cast<float>(preview.frameCount));
        const auto endX = inner.getX() + inner.getWidth()
            * (static_cast<float>(preview.loopEndFrame) / static_cast<float>(preview.frameCount));
        g.setColour(authoringPanelAccent);
        g.drawVerticalLine(static_cast<int>(startX), inner.getY(), inner.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), inner.getY(), inner.getBottom());
    }
}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);

    auto hero = area.removeFromTop(76);
    auto heroLeft = hero.removeFromLeft(hero.proportionOfWidth(0.62f));
    titleLabel.setBounds(heroLeft.removeFromTop(30));
    heroLeft.removeFromTop(6);
    statusLabel.setBounds(heroLeft.removeFromTop(20));
    sourceLabel.setBounds(heroLeft.removeFromTop(20));
    articulationLabel.setBounds(heroLeft.removeFromTop(20));

    auto heroButtons = hero.removeFromRight(320);
    auto topRow = heroButtons.removeFromTop(28);
    undoButton.setBounds(topRow.removeFromLeft(92));
    topRow.removeFromLeft(8);
    redoButton.setBounds(topRow.removeFromLeft(92));
    topRow.removeFromLeft(8);
    saveCheckpointButton.setBounds(topRow.removeFromLeft(120));
    heroButtons.removeFromTop(10);
    previewButton.setBounds(heroButtons.removeFromTop(30));

    area.removeFromTop(12);
    zoneLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);
    zoneSelector.setBounds(area.removeFromTop(28).removeFromLeft(360));

    area.removeFromTop(12);
    waveformLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);
    waveformPreview.setBounds(area.removeFromTop(150));
    area.removeFromTop(6);
    waveformInfoLabel.setBounds(area.removeFromTop(22));
    loopInfoLabel.setBounds(area.removeFromTop(22));
    importMetricsLabel.setBounds(area.removeFromTop(38));

    area.removeFromTop(10);
    auto modeRow = area.removeFromTop(28);
    inspectorModeLabel.setBounds(modeRow.removeFromLeft(72));
    modeRow.removeFromLeft(8);
    inspectorModeSelector.setBounds(modeRow.removeFromLeft(210));

    area.removeFromTop(12);
    auto inspector = area;
    const auto expanded = isExpandedLayout(layoutMode);

    if (inspectorModeSelector.getSelectedId() == 1)
    {
        zoneMap.setBounds(inspector.removeFromTop(expanded ? 190 : 160));
        inspector.removeFromTop(12);

        auto leftColumn = inspector.removeFromLeft(inspector.proportionOfWidth(0.5f));
        auto rightColumn = inspector;

        auto layoutSliderRow = [](juce::Rectangle<int>& column, juce::Label& label, juce::Slider& slider)
        {
            auto row = column.removeFromTop(30);
            label.setBounds(row.removeFromLeft(120));
            slider.setBounds(row);
            column.removeFromTop(8);
        };

        layoutSliderRow(leftColumn, rootKeyLabel, rootKeySlider);
        layoutSliderRow(leftColumn, keyLowLabel, keyLowSlider);
        layoutSliderRow(leftColumn, keyHighLabel, keyHighSlider);
        layoutSliderRow(leftColumn, velocityLowLabel, velocityLowSlider);
        layoutSliderRow(leftColumn, velocityHighLabel, velocityHighSlider);

        layoutSliderRow(rightColumn, gainLabel, gainSlider);
        layoutSliderRow(rightColumn, panLabel, panSlider);
        loopEnabledToggle.setBounds(rightColumn.removeFromTop(28));
    }
    else if (inspectorModeSelector.getSelectedId() == 2)
    {
        macroSectionLabel.setBounds(inspector.removeFromTop(24));
        inspector.removeFromTop(6);

        auto selectorRow = inspector.removeFromTop(28);
        macroSelector.setBounds(selectorRow.removeFromLeft(260));
        selectorRow.removeFromLeft(8);
        macroMoveUpButton.setBounds(selectorRow.removeFromLeft(90));
        selectorRow.removeFromLeft(8);
        macroMoveDownButton.setBounds(selectorRow.removeFromLeft(90));

        inspector.removeFromTop(10);

        auto layoutSliderRow = [](juce::Rectangle<int>& section,
                                  juce::Label& label,
                                  juce::Component& field)
        {
            auto row = section.removeFromTop(30);
            label.setBounds(row.removeFromLeft(120));
            field.setBounds(row);
            section.removeFromTop(8);
        };

        layoutSliderRow(inspector, macroAssignmentLabel, macroAssignmentSelector);
        layoutSliderRow(inspector, macroRoleLabel, macroRoleSelector);
        layoutSliderRow(inspector, macroDefaultLabel, macroDefaultSlider);
        layoutSliderRow(inspector, macroMinLabel, macroMinSlider);
        layoutSliderRow(inspector, macroMaxLabel, macroMaxSlider);

        if (expanded)
        {
            inspector.removeFromTop(6);
            macroSummaryLabel.setBounds(inspector.removeFromTop(42));
        }
    }
    else if (inspectorModeSelector.getSelectedId() == 3)
    {
        auto topSection = inspector.removeFromTop(expanded ? 122 : 96);
        fxSectionLabel.setBounds(topSection.removeFromTop(24));
        topSection.removeFromTop(6);

        auto fxSelectorRow = topSection.removeFromTop(28);
        fxSelector.setBounds(fxSelectorRow.removeFromLeft(260));
        topSection.removeFromTop(8);

        auto fxTypeRow = topSection.removeFromTop(30);
        fxTypeLabel.setBounds(fxTypeRow.removeFromLeft(120));
        fxTypeSelector.setBounds(fxTypeRow);
        topSection.removeFromTop(8);
        fxBypassedToggle.setBounds(topSection.removeFromTop(28));

        if (expanded)
        {
            topSection.removeFromTop(6);
            fxSummaryLabel.setBounds(topSection.removeFromTop(34));
        }

        inspector.removeFromTop(12);
        routingSectionLabel.setBounds(inspector.removeFromTop(24));
        inspector.removeFromTop(6);
        routingBusSelector.setBounds(inspector.removeFromTop(28).removeFromLeft(260));
        inspector.removeFromTop(10);

        auto layoutComboRow = [](juce::Rectangle<int>& section,
                                 juce::Label& label,
                                 juce::ComboBox& combo)
        {
            auto row = section.removeFromTop(30);
            label.setBounds(row.removeFromLeft(120));
            combo.setBounds(row);
            section.removeFromTop(8);
        };

        layoutComboRow(inspector, routingInputLabel, routingInputSelector);
        layoutComboRow(inspector, routingInsertOneLabel, routingInsertOneSelector);
        layoutComboRow(inspector, routingInsertTwoLabel, routingInsertTwoSelector);

        if (expanded)
        {
            inspector.removeFromTop(6);
            routingSummaryLabel.setBounds(inspector.removeFromTop(42));
        }
    }
    else
    {
        performanceSectionLabel.setBounds(inspector.removeFromTop(24));
        inspector.removeFromTop(6);

        auto selectorRow = inspector.removeFromTop(28);
        performanceBankSelector.setBounds(selectorRow.removeFromLeft(250));
        selectorRow.removeFromLeft(10);
        triggerSlotSelector.setBounds(selectorRow.removeFromLeft(250));

        inspector.removeFromTop(10);

        auto layoutComboRow = [](juce::Rectangle<int>& section,
                                 juce::Label& label,
                                 juce::Component& field)
        {
            auto row = section.removeFromTop(30);
            label.setBounds(row.removeFromLeft(120));
            field.setBounds(row);
            section.removeFromTop(8);
        };

        layoutComboRow(inspector, triggerEventLabel, triggerEventSelector);
        layoutComboRow(inspector, targetArticulationLabel, targetArticulationSelector);
        layoutComboRow(inspector, phraseAssetLabel, phraseAssetSelector);
        layoutComboRow(inspector, chordModeLabel, chordModeSelector);
        layoutComboRow(inspector, phraseImportPathLabel, phraseImportPathEditor);

        auto importRow = inspector.removeFromTop(30);
        phraseImportButton.setBounds(importRow.removeFromLeft(180));

        if (expanded)
        {
            inspector.removeFromTop(10);
            performanceSummaryLabel.setBounds(inspector.removeFromTop(42));
            phraseSummaryLabel.setBounds(inspector.removeFromTop(60));
        }
    }
}

void AuthoringPanel::reloadFromSession()
{
    refreshFromSession();
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

void AuthoringPanel::refreshInspectorVisibility()
{
    const auto mappingMode = inspectorModeSelector.getSelectedId() == 1;
    const auto macroMode = inspectorModeSelector.getSelectedId() == 2;
    const auto routingMode = inspectorModeSelector.getSelectedId() == 3;
    const auto performanceMode = inspectorModeSelector.getSelectedId() == 4;
    const auto expanded = isExpandedLayout(layoutMode);

    zoneMap.setVisible(mappingMode);
    rootKeyLabel.setVisible(mappingMode);
    rootKeySlider.setVisible(mappingMode);
    keyLowLabel.setVisible(mappingMode);
    keyLowSlider.setVisible(mappingMode);
    keyHighLabel.setVisible(mappingMode);
    keyHighSlider.setVisible(mappingMode);
    velocityLowLabel.setVisible(mappingMode);
    velocityLowSlider.setVisible(mappingMode);
    velocityHighLabel.setVisible(mappingMode);
    velocityHighSlider.setVisible(mappingMode);
    gainLabel.setVisible(mappingMode);
    gainSlider.setVisible(mappingMode);
    panLabel.setVisible(mappingMode);
    panSlider.setVisible(mappingMode);
    loopEnabledToggle.setVisible(mappingMode);

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
    const auto& documentState = authoringSession.getDocumentState();
    statusLabel.setText("Revision " + juce::String(static_cast<int>(documentState.revision))
                            + " | dirty=" + juce::String(documentState.dirty ? "yes" : "no")
                            + " | undo=" + juce::String(static_cast<int>(documentState.undoDepth))
                            + " | redo=" + juce::String(static_cast<int>(documentState.redoDepth)),
                        juce::dontSendNotification);

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        sourceLabel.setText("Sample source: " + juce::String::fromUTF8(zone->sampleSourceId.c_str()),
                            juce::dontSendNotification);
        articulationLabel.setText("Articulation: " + juce::String::fromUTF8(zone->articulationId.c_str()),
                                  juce::dontSendNotification);

        rootKeySlider.setValue(zone->rootKey, juce::dontSendNotification);
        keyLowSlider.setValue(zone->keyLow, juce::dontSendNotification);
        keyHighSlider.setValue(zone->keyHigh, juce::dontSendNotification);
        velocityLowSlider.setValue(zone->velocityLow, juce::dontSendNotification);
        velocityHighSlider.setValue(zone->velocityHigh, juce::dontSendNotification);
        gainSlider.setValue(zone->gainDb, juce::dontSendNotification);
        panSlider.setValue(zone->pan, juce::dontSendNotification);
        loopEnabledToggle.setToggleState(zone->loopEnabled, juce::dontSendNotification);
        previewButton.setEnabled(true);
    }
    else
    {
        sourceLabel.setText("Sample source: none", juce::dontSendNotification);
        articulationLabel.setText("Articulation: none", juce::dontSendNotification);
        previewButton.setEnabled(false);
    }

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
    undoButton.setEnabled(documentState.undoDepth > 0);
    redoButton.setEnabled(documentState.redoDepth > 0);
}

void AuthoringPanel::applySelectedZoneEdit(const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    auto editedZone = *currentZone;
    editedZone.rootKey = static_cast<int>(rootKeySlider.getValue());
    editedZone.keyLow = static_cast<int>(keyLowSlider.getValue());
    editedZone.keyHigh = static_cast<int>(keyHighSlider.getValue());
    editedZone.velocityLow = static_cast<int>(velocityLowSlider.getValue());
    editedZone.velocityHigh = static_cast<int>(velocityHighSlider.getValue());
    editedZone.gainDb = gainSlider.getValue();
    editedZone.pan = panSlider.getValue();
    editedZone.loopEnabled = loopEnabledToggle.getToggleState();

    authoringSession.updateSelectedZone(editedZone, label.toStdString());
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
