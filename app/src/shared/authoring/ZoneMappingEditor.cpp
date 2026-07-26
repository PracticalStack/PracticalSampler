#include "shared/authoring/ZoneMappingEditor.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
constexpr int sectionGap = 8;
constexpr int sliderRowHeight = 44;
constexpr int rangeRowHeight = 58;
constexpr int toggleRowHeight = 24;
constexpr int comboRowHeight = 28;
constexpr int actionRowHeight = 24;
constexpr int messageRowHeight = 20;
constexpr int roundRobinMessageRowHeight = 20;

void addOwnedRow(juce::Component& parent, juce::Component& child, int height)
{
    parent.addAndMakeVisible(child);
    child.setSize(0, height);
}

void setVisibleAndAccessible(juce::Component& component, bool shouldShow)
{
    component.setVisible(shouldShow);
    component.setAccessible(shouldShow);
}
} // namespace

ZoneMappingEditor::ZoneMappingEditor()
    : emptyStateMessage("authoringZoneFieldEmptyState", juce::Justification::centred),
      mapSection("Map", "authoringMapInspectorSection", true),
      sampleSection("Sample", "authoringSampleInspectorSection", false),
      roundRobinSection("Round Robin", "authoringRoundRobinInspectorSection", false),
      mixSection("Mix", "authoringMixInspectorSection", false),
      advancedSection("Advanced", "authoringAdvancedInspectorSection", false),
      rootKeyRow("Root Key", "authoringRootKeyRow", 0, 127, 1),
      keyRangeRow("Key Range", "authoringKeyRangeRow", "Low", "High", 0, 127, 1),
      velocityRangeRow("Velocity Range", "authoringVelocityRangeRow", "Low", "High", 1, 127, 1),
      roundRobinPoolMessage("authoringRoundRobinPoolMessage", juce::Justification::centredLeft),
      roundRobinSlotMessage("authoringRoundRobinSlotMessage", juce::Justification::centredLeft),
      roundRobinHintMessage("authoringRoundRobinHintMessage", juce::Justification::centredLeft),
      createRoundRobinPoolRow("Pool", "authoringCreateRoundRobinPoolRow", "Create Pool"),
      addCompatibleZonesRow("Grouping", "authoringAddCompatibleZonesRow", "Add Matching Zones"),
      normalizeRoundRobinPoolRow("Repair", "authoringNormalizeRoundRobinPoolRow", "Normalize Slots"),
      removeRoundRobinPoolRow("Remove", "authoringRemoveRoundRobinPoolRow", "Remove / Reindex"),
      gainRow("Gain (dB)", "authoringGainRow", -24.0, 12.0, 0.1),
      panRow("Pan", "authoringPanRow", -1.0, 1.0, 0.01),
      loopToggleRow("Loop", "authoringLoopRow", "Enabled"),
      triggerModeRow("Trigger mode", "authoringTriggerModeRow"),
      previewZoneRow("Audition", "authoringInspectorPreviewRow", "Preview Zone"),
      restoreRootKeyRow("Reference", "authoringRestoreRootKeyRow", "Restore Root Key"),
      validationMessage("authoringZoneValidationMessage", juce::Justification::centredLeft)
{
    setComponentID("authoringZoneFieldEditor");
    setTitle("Zone mapping inspector");
    setDescription("Edits the selected zone mapping, sample, mix, and advanced settings.");

    emptyStateMessage.setText("Select a zone to edit mapping fields.");
    validationMessage.setText("Ranges are normalized on commit to keep low and high values valid.");

    rootKeyRow.getSlider().setComponentID("authoringRootKeySlider");
    keyRangeRow.getLowSlider().setComponentID("authoringKeyLowSlider");
    keyRangeRow.getHighSlider().setComponentID("authoringKeyHighSlider");
    velocityRangeRow.getLowSlider().setComponentID("authoringVelocityLowSlider");
    velocityRangeRow.getHighSlider().setComponentID("authoringVelocityHighSlider");
    gainRow.getSlider().setComponentID("authoringGainSlider");
    panRow.getSlider().setComponentID("authoringPanSlider");
    loopToggleRow.getToggle().setComponentID("authoringLoopEnabledToggle");
    triggerModeRow.getComboBox().setComponentID("authoringTriggerModeSelector");
    triggerModeRow.getComboBox().addItem("Gated", 1);
    triggerModeRow.getComboBox().addItem("One-shot", 2);
    createRoundRobinPoolRow.getButton().setComponentID("authoringCreateRoundRobinPoolButton");
    addCompatibleZonesRow.getButton().setComponentID("authoringAddCompatibleZonesButton");
    normalizeRoundRobinPoolRow.getButton().setComponentID("authoringNormalizeRoundRobinPoolButton");
    removeRoundRobinPoolRow.getButton().setComponentID("authoringRemoveRoundRobinPoolButton");
    previewZoneRow.getButton().setComponentID("authoringInspectorPreviewButton");
    restoreRootKeyRow.getButton().setComponentID("authoringRestoreRootKeyButton");
    mapSection.getDisclosureButton().setExplicitFocusOrder(40);
    rootKeyRow.getSlider().setExplicitFocusOrder(41);
    keyRangeRow.getLowSlider().setExplicitFocusOrder(42);
    keyRangeRow.getHighSlider().setExplicitFocusOrder(43);
    sampleSection.getDisclosureButton().setExplicitFocusOrder(44);
    velocityRangeRow.getLowSlider().setExplicitFocusOrder(45);
    velocityRangeRow.getHighSlider().setExplicitFocusOrder(46);
    roundRobinSection.getDisclosureButton().setExplicitFocusOrder(47);
    createRoundRobinPoolRow.getButton().setExplicitFocusOrder(48);
    addCompatibleZonesRow.getButton().setExplicitFocusOrder(49);
    normalizeRoundRobinPoolRow.getButton().setExplicitFocusOrder(50);
    removeRoundRobinPoolRow.getButton().setExplicitFocusOrder(51);
    mixSection.getDisclosureButton().setExplicitFocusOrder(52);
    gainRow.getSlider().setExplicitFocusOrder(53);
    panRow.getSlider().setExplicitFocusOrder(54);
    advancedSection.getDisclosureButton().setExplicitFocusOrder(55);
    loopToggleRow.getToggle().setExplicitFocusOrder(56);
    triggerModeRow.getComboBox().setExplicitFocusOrder(57);
    previewZoneRow.getButton().setExplicitFocusOrder(58);
    restoreRootKeyRow.getButton().setExplicitFocusOrder(59);
    triggerModeRow.getComboBox().setHelpText(
        "Gated samples release on note-off. One-shot samples play to their natural end.");
    previewZoneRow.getButton().setHelpText("Auditions the selected zone from the mapping inspector.");
    restoreRootKeyRow.getButton().setHelpText(
        "Restores the selected zone root key from the imported sample reference pitch.");
    createRoundRobinPoolRow.getButton().setHelpText(
        "Creates a dedicated sequential Round Robin pool for the selected zone.");
    addCompatibleZonesRow.getButton().setHelpText(
        "Adds compatible unpooled zones into the selected zone's sequential Round Robin pool.");
    normalizeRoundRobinPoolRow.getButton().setHelpText(
        "Renumbers the selected Round Robin pool so slots are dense and ordered.");
    removeRoundRobinPoolRow.getButton().setHelpText(
        "Removes the selected zone from its Round Robin pool and explicitly reindexes remaining peers.");

    addOwnedRow(mapSectionContent, rootKeyRow, sliderRowHeight);
    addOwnedRow(mapSectionContent, keyRangeRow, rangeRowHeight);
    mapSectionContent.setSize(0, sliderRowHeight + 6 + rangeRowHeight);

    addOwnedRow(sampleSectionContent, velocityRangeRow, rangeRowHeight);
    sampleSectionContent.setSize(0, rangeRowHeight);

    addOwnedRow(roundRobinSectionContent, roundRobinPoolMessage, roundRobinMessageRowHeight);
    addOwnedRow(roundRobinSectionContent, roundRobinSlotMessage, roundRobinMessageRowHeight);
    addOwnedRow(roundRobinSectionContent, roundRobinHintMessage, roundRobinMessageRowHeight);
    addOwnedRow(roundRobinSectionContent, createRoundRobinPoolRow, actionRowHeight);
    addOwnedRow(roundRobinSectionContent, addCompatibleZonesRow, actionRowHeight);
    addOwnedRow(roundRobinSectionContent, normalizeRoundRobinPoolRow, actionRowHeight);
    addOwnedRow(roundRobinSectionContent, removeRoundRobinPoolRow, actionRowHeight);
    roundRobinSectionContent.setSize(0, roundRobinMessageRowHeight + 6
                                        + roundRobinMessageRowHeight + 6
                                        + roundRobinMessageRowHeight + 6
                                        + actionRowHeight + 6
                                        + actionRowHeight + 6
                                        + actionRowHeight + 6
                                        + actionRowHeight);

    addOwnedRow(mixSectionContent, gainRow, sliderRowHeight);
    addOwnedRow(mixSectionContent, panRow, sliderRowHeight);
    mixSectionContent.setSize(0, sliderRowHeight + 6 + sliderRowHeight);

    addOwnedRow(advancedSectionContent, loopToggleRow, toggleRowHeight);
    addOwnedRow(advancedSectionContent, triggerModeRow, comboRowHeight);
    addOwnedRow(advancedSectionContent, previewZoneRow, actionRowHeight);
    addOwnedRow(advancedSectionContent, restoreRootKeyRow, actionRowHeight);
    addOwnedRow(advancedSectionContent, validationMessage, messageRowHeight);
    advancedSectionContent.setSize(0, toggleRowHeight + 6 + comboRowHeight + 6 + actionRowHeight + 6
                                      + actionRowHeight + 6 + messageRowHeight);

    mapSection.setContent(&mapSectionContent);
    sampleSection.setContent(&sampleSectionContent);
    roundRobinSection.setContent(&roundRobinSectionContent);
    mixSection.setContent(&mixSectionContent);
    advancedSection.setContent(&advancedSectionContent);
    mapSection.setOnExpandedChanged([this](bool) { resized(); });
    sampleSection.setOnExpandedChanged([this](bool) { resized(); });
    roundRobinSection.setOnExpandedChanged([this](bool) { resized(); });
    mixSection.setOnExpandedChanged([this](bool) { resized(); });
    advancedSection.setOnExpandedChanged([this](bool) { resized(); });

    auto bindCommitOnGestureFinished = [this](CompactInspectorCommitSlider& slider, const char* labelText)
    {
        slider.setOnCommitFinished([this, label = juce::String(labelText)](CompactInspectorCommitSlider::CommitSource)
        {
            commitCurrentValues(label);
        });
    };

    bindCommitOnGestureFinished(rootKeyRow.getSlider(), "Update zone root key");
    bindCommitOnGestureFinished(keyRangeRow.getLowSlider(), "Update zone key range");
    bindCommitOnGestureFinished(keyRangeRow.getHighSlider(), "Update zone key range");
    bindCommitOnGestureFinished(velocityRangeRow.getLowSlider(), "Update zone velocity range");
    bindCommitOnGestureFinished(velocityRangeRow.getHighSlider(), "Update zone velocity range");
    bindCommitOnGestureFinished(gainRow.getSlider(), "Update zone gain");
    bindCommitOnGestureFinished(panRow.getSlider(), "Update zone pan");

    loopToggleRow.getToggle().onClick = [this]
    {
        commitCurrentValues("Toggle zone loop");
    };

    triggerModeRow.getComboBox().onChange = [this]
    {
        commitCurrentValues("Update zone trigger mode");
    };

    restoreRootKeyRow.getButton().onClick = [this]
    {
        if (callbacks.onRestoreRootKeyRequested)
            callbacks.onRestoreRootKeyRequested();
    };
    previewZoneRow.getButton().onClick = [this]
    {
        if (callbacks.onPreviewRequested)
            callbacks.onPreviewRequested();
    };
    createRoundRobinPoolRow.getButton().onClick = [this]
    {
        if (callbacks.onCreateRoundRobinPoolRequested)
            callbacks.onCreateRoundRobinPoolRequested();
    };
    addCompatibleZonesRow.getButton().onClick = [this]
    {
        if (callbacks.onAddCompatibleZonesToRoundRobinPoolRequested)
            callbacks.onAddCompatibleZonesToRoundRobinPoolRequested();
    };
    normalizeRoundRobinPoolRow.getButton().onClick = [this]
    {
        if (callbacks.onNormalizeRoundRobinPoolRequested)
            callbacks.onNormalizeRoundRobinPoolRequested();
    };
    removeRoundRobinPoolRow.getButton().onClick = [this]
    {
        if (callbacks.onRemoveSelectedZoneFromRoundRobinPoolRequested)
            callbacks.onRemoveSelectedZoneFromRoundRobinPoolRequested();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&emptyStateMessage),
             static_cast<juce::Component*>(&mapSection),
             static_cast<juce::Component*>(&sampleSection),
             static_cast<juce::Component*>(&roundRobinSection),
             static_cast<juce::Component*>(&mixSection),
             static_cast<juce::Component*>(&advancedSection)
         })
    {
        addAndMakeVisible(component);
    }
}

void ZoneMappingEditor::resized()
{
    auto area = getLocalBounds();

    if (!viewModel.hasSelection)
    {
        emptyStateMessage.setBounds(area);
        return;
    }

    auto layoutSection = [&](CompactInspectorSection& section, bool addGap)
    {
        const auto sectionHeight = section.getPreferredHeight();
        section.setBounds(area.removeFromTop(sectionHeight));
        if (addGap)
            area.removeFromTop(sectionGap);
    };

    layoutSection(mapSection, true);
    layoutSection(sampleSection, true);
    layoutSection(roundRobinSection, true);
    layoutSection(mixSection, true);
    layoutSection(advancedSection, false);

    auto mapArea = mapSectionContent.getLocalBounds();
    rootKeyRow.setBounds(mapArea.removeFromTop(sliderRowHeight));
    mapArea.removeFromTop(6);
    keyRangeRow.setBounds(mapArea.removeFromTop(rangeRowHeight));

    auto sampleArea = sampleSectionContent.getLocalBounds();
    velocityRangeRow.setBounds(sampleArea.removeFromTop(rangeRowHeight));

    auto roundRobinArea = roundRobinSectionContent.getLocalBounds();
    roundRobinPoolMessage.setBounds(roundRobinArea.removeFromTop(roundRobinMessageRowHeight));
    roundRobinArea.removeFromTop(6);
    roundRobinSlotMessage.setBounds(roundRobinArea.removeFromTop(roundRobinMessageRowHeight));
    roundRobinArea.removeFromTop(6);
    roundRobinHintMessage.setBounds(roundRobinArea.removeFromTop(roundRobinMessageRowHeight));
    roundRobinArea.removeFromTop(6);
    createRoundRobinPoolRow.setBounds(roundRobinArea.removeFromTop(actionRowHeight));
    roundRobinArea.removeFromTop(6);
    addCompatibleZonesRow.setBounds(roundRobinArea.removeFromTop(actionRowHeight));
    roundRobinArea.removeFromTop(6);
    normalizeRoundRobinPoolRow.setBounds(roundRobinArea.removeFromTop(actionRowHeight));
    roundRobinArea.removeFromTop(6);
    removeRoundRobinPoolRow.setBounds(roundRobinArea.removeFromTop(actionRowHeight));

    auto mixArea = mixSectionContent.getLocalBounds();
    gainRow.setBounds(mixArea.removeFromTop(sliderRowHeight));
    mixArea.removeFromTop(6);
    panRow.setBounds(mixArea.removeFromTop(sliderRowHeight));

    auto advancedArea = advancedSectionContent.getLocalBounds();
    loopToggleRow.setBounds(advancedArea.removeFromTop(toggleRowHeight));
    advancedArea.removeFromTop(6);
    triggerModeRow.setBounds(advancedArea.removeFromTop(comboRowHeight));
    advancedArea.removeFromTop(6);
    previewZoneRow.setBounds(advancedArea.removeFromTop(actionRowHeight));
    advancedArea.removeFromTop(6);
    restoreRootKeyRow.setBounds(advancedArea.removeFromTop(actionRowHeight));
    advancedArea.removeFromTop(6);
    validationMessage.setBounds(advancedArea.removeFromTop(messageRowHeight));
}

void ZoneMappingEditor::setViewModel(ZoneFieldValuesViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    emptyStateMessage.setText(juce::String::fromUTF8(viewModel.emptyStateText.c_str()));

    applyValuesToControls(viewModel);
    roundRobinPoolMessage.setText(juce::String::fromUTF8(viewModel.roundRobinPoolText.c_str()));
    roundRobinSlotMessage.setText(juce::String::fromUTF8(viewModel.roundRobinSlotText.c_str()));
    roundRobinHintMessage.setText(juce::String::fromUTF8(viewModel.roundRobinHintText.c_str()));

    const auto hasSelection = viewModel.hasSelection;

    setVisibleAndAccessible(emptyStateMessage, !hasSelection);
    for (auto* component : {
             static_cast<juce::Component*>(&mapSection),
             static_cast<juce::Component*>(&sampleSection),
             static_cast<juce::Component*>(&roundRobinSection),
             static_cast<juce::Component*>(&mixSection),
             static_cast<juce::Component*>(&advancedSection),
             static_cast<juce::Component*>(&mapSectionContent),
             static_cast<juce::Component*>(&sampleSectionContent),
             static_cast<juce::Component*>(&roundRobinSectionContent),
             static_cast<juce::Component*>(&mixSectionContent),
             static_cast<juce::Component*>(&advancedSectionContent),
             static_cast<juce::Component*>(&rootKeyRow),
             static_cast<juce::Component*>(&keyRangeRow),
             static_cast<juce::Component*>(&velocityRangeRow),
             static_cast<juce::Component*>(&roundRobinPoolMessage),
             static_cast<juce::Component*>(&roundRobinSlotMessage),
             static_cast<juce::Component*>(&roundRobinHintMessage),
             static_cast<juce::Component*>(&createRoundRobinPoolRow),
             static_cast<juce::Component*>(&addCompatibleZonesRow),
             static_cast<juce::Component*>(&normalizeRoundRobinPoolRow),
             static_cast<juce::Component*>(&removeRoundRobinPoolRow),
             static_cast<juce::Component*>(&gainRow),
             static_cast<juce::Component*>(&panRow),
             static_cast<juce::Component*>(&loopToggleRow),
             static_cast<juce::Component*>(&triggerModeRow),
             static_cast<juce::Component*>(&previewZoneRow),
             static_cast<juce::Component*>(&restoreRootKeyRow),
             static_cast<juce::Component*>(&validationMessage),
             static_cast<juce::Component*>(&rootKeyRow.getSlider()),
             static_cast<juce::Component*>(&keyRangeRow.getLowSlider()),
             static_cast<juce::Component*>(&keyRangeRow.getHighSlider()),
             static_cast<juce::Component*>(&velocityRangeRow.getLowSlider()),
             static_cast<juce::Component*>(&velocityRangeRow.getHighSlider()),
             static_cast<juce::Component*>(&gainRow.getSlider()),
             static_cast<juce::Component*>(&panRow.getSlider()),
             static_cast<juce::Component*>(&loopToggleRow.getToggle()),
             static_cast<juce::Component*>(&triggerModeRow.getComboBox()),
             static_cast<juce::Component*>(&createRoundRobinPoolRow.getButton()),
             static_cast<juce::Component*>(&addCompatibleZonesRow.getButton()),
             static_cast<juce::Component*>(&normalizeRoundRobinPoolRow.getButton()),
             static_cast<juce::Component*>(&removeRoundRobinPoolRow.getButton()),
             static_cast<juce::Component*>(&previewZoneRow.getButton()),
             static_cast<juce::Component*>(&restoreRootKeyRow.getButton())
         })
    {
        setVisibleAndAccessible(*component, hasSelection);
    }

    restoreRootKeyRow.getButton().setEnabled(viewModel.hasSelection);
    previewZoneRow.getButton().setEnabled(viewModel.hasSelection);
    createRoundRobinPoolRow.getButton().setButtonText(viewModel.roundRobinEnabled
                                                          ? "Split to New Pool"
                                                          : "Create Pool");
    createRoundRobinPoolRow.getButton().setHelpText(viewModel.roundRobinEnabled
                                                        ? "Moves the selected zone into a new Round Robin pool and explicitly reindexes the previous pool."
                                                        : "Creates a dedicated sequential Round Robin pool for the selected zone.");
    previewZoneRow.getButton().setButtonText(viewModel.previewAdvancesRoundRobin
                                                 ? "Preview / Advance"
                                                 : "Preview Zone");
    previewZoneRow.getButton().setHelpText(viewModel.previewAdvancesRoundRobin
                                               ? "Auditions the selected zone and advances its active Round Robin slot."
                                               : "Auditions the selected zone from the mapping inspector.");
    createRoundRobinPoolRow.getButton().setEnabled(viewModel.hasSelection
                                                   && viewModel.canCreateRoundRobinPool);
    addCompatibleZonesRow.getButton().setEnabled(viewModel.hasSelection
                                                 && viewModel.canAddCompatibleZonesToRoundRobinPool);
    normalizeRoundRobinPoolRow.getButton().setEnabled(viewModel.hasSelection
                                                      && viewModel.canNormalizeRoundRobinPool);
    removeRoundRobinPoolRow.getButton().setEnabled(viewModel.hasSelection
                                                   && viewModel.canRemoveZoneFromRoundRobinPool);
    refreshValidationMessage({});
    resized();
}

void ZoneMappingEditor::setCallbacks(ZoneFieldCallbacks nextCallbacks)
{
    callbacks = std::move(nextCallbacks);
}

ZoneMappingEditor::CommitValues ZoneMappingEditor::collectCurrentValues() const
{
    CommitValues commitValues;
    auto& values = commitValues.values;
    values = viewModel;
    values.hasSelection = true;
    values.rootKey = static_cast<int>(rootKeyRow.getSlider().getValue());
    values.keyLow = static_cast<int>(keyRangeRow.getLowSlider().getValue());
    values.keyHigh = static_cast<int>(keyRangeRow.getHighSlider().getValue());
    values.velocityLow = static_cast<int>(velocityRangeRow.getLowSlider().getValue());
    values.velocityHigh = static_cast<int>(velocityRangeRow.getHighSlider().getValue());
    values.gainDb = gainRow.getSlider().getValue();
    values.pan = panRow.getSlider().getValue();
    values.loopEnabled = loopToggleRow.getToggle().getToggleState();
    values.triggerMode = triggerModeRow.getComboBox().getSelectedId() == 2
        ? drs::engine::ZoneTriggerMode::oneShot
        : drs::engine::ZoneTriggerMode::gated;

    const auto normalizedKeyRange = values.keyLow > values.keyHigh;
    const auto normalizedVelocityRange = values.velocityLow > values.velocityHigh;

    if (normalizedKeyRange)
        std::swap(values.keyLow, values.keyHigh);
    if (normalizedVelocityRange)
        std::swap(values.velocityLow, values.velocityHigh);

    if (normalizedKeyRange && normalizedVelocityRange)
        commitValues.validationMessage = "Key and velocity ranges were normalized to keep Low <= High.";
    else if (normalizedKeyRange)
        commitValues.validationMessage = "Key range was normalized to keep Low <= High.";
    else if (normalizedVelocityRange)
        commitValues.validationMessage = "Velocity range was normalized to keep Low <= High.";

    return commitValues;
}

void ZoneMappingEditor::commitCurrentValues(const juce::String& label)
{
    if (!callbacks.onCommitRequested || !viewModel.hasSelection)
        return;

    auto commitValues = collectCurrentValues();
    viewModel = commitValues.values;
    applyValuesToControls(viewModel);
    refreshValidationMessage(commitValues.validationMessage);
    callbacks.onCommitRequested(viewModel, label.toStdString());
}

void ZoneMappingEditor::applyValuesToControls(const ZoneFieldValuesViewModel& values)
{
    rootKeyRow.getSlider().setValue(values.rootKey, juce::dontSendNotification);
    keyRangeRow.getLowSlider().setValue(values.keyLow, juce::dontSendNotification);
    keyRangeRow.getHighSlider().setValue(values.keyHigh, juce::dontSendNotification);
    velocityRangeRow.getLowSlider().setValue(values.velocityLow, juce::dontSendNotification);
    velocityRangeRow.getHighSlider().setValue(values.velocityHigh, juce::dontSendNotification);
    gainRow.getSlider().setValue(values.gainDb, juce::dontSendNotification);
    panRow.getSlider().setValue(values.pan, juce::dontSendNotification);
    loopToggleRow.getToggle().setToggleState(values.loopEnabled, juce::dontSendNotification);
    triggerModeRow.getComboBox().setSelectedId(
        values.triggerMode == drs::engine::ZoneTriggerMode::oneShot ? 2 : 1,
        juce::dontSendNotification);
}

void ZoneMappingEditor::refreshValidationMessage(const juce::String& messageText)
{
    validationMessage.setText(messageText.isEmpty()
                                  ? "Ranges are normalized on commit to keep low and high values valid."
                                  : messageText);
}
} // namespace drs::app::authoring
