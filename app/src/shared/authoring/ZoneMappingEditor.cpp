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
      performanceSection("Performance", "authoringPerformanceInspectorSection", false),
      rootKeyRow("Root Key", "authoringRootKeyRow", 0, 127, 1),
      keyRangeRow("Key Range", "authoringKeyRangeRow", "Low", "High", 0, 127, 1),
      articulationRow("Articulation", "authoringZoneArticulationRow"),
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
      performanceEventRow("Event", "authoringPerformanceEventRow"),
      sustainConditionRow("Sustain", "authoringSustainConditionRow"),
      pitchSourceRow("Pitch", "authoringPitchSourceRow"),
      chokeGroupRow("Choke group", "authoringChokeGroupRow"),
      chokeTargetRow("Choke target", "authoringChokeTargetRow"),
      chokeFadeRow("Choke fade", "authoringChokeFadeRow", 0.0, 5.0, 0.01),
      createChokeGroupRow("Groups", "authoringCreateChokeGroupRow", "New Choke Group"),
      performanceHintMessage("authoringPerformanceHintMessage", juce::Justification::centredLeft),
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
    articulationRow.getComboBox().setComponentID("authoringZoneArticulationSelector");
    velocityRangeRow.getLowSlider().setComponentID("authoringVelocityLowSlider");
    velocityRangeRow.getHighSlider().setComponentID("authoringVelocityHighSlider");
    gainRow.getSlider().setComponentID("authoringGainSlider");
    panRow.getSlider().setComponentID("authoringPanSlider");
    loopToggleRow.getToggle().setComponentID("authoringLoopEnabledToggle");
    triggerModeRow.getComboBox().setComponentID("authoringTriggerModeSelector");
    performanceEventRow.getComboBox().setComponentID("authoringPerformanceEventSelector");
    sustainConditionRow.getComboBox().setComponentID("authoringPerformanceSustainSelector");
    pitchSourceRow.getComboBox().setComponentID("authoringPerformancePitchSelector");
    chokeGroupRow.getComboBox().setComponentID("authoringChokeGroupSelector");
    chokeTargetRow.getComboBox().setComponentID("authoringChokeTargetSelector");
    chokeFadeRow.getSlider().setComponentID("authoringChokeFadeSlider");
    createChokeGroupRow.getButton().setComponentID("authoringCreateChokeGroupButton");
    triggerModeRow.getComboBox().addItem("Gated", 1);
    triggerModeRow.getComboBox().addItem("One-shot", 2);
    performanceEventRow.getComboBox().addItem("Note On", 1);
    performanceEventRow.getComboBox().addItem("Note Off", 2);
    performanceEventRow.getComboBox().addItem("Release", 3);
    performanceEventRow.getComboBox().addItem("Pedal Down", 4);
    performanceEventRow.getComboBox().addItem("Pedal Up", 5);
    sustainConditionRow.getComboBox().addItem("Any pedal state", 1);
    sustainConditionRow.getComboBox().addItem("Pedal down", 2);
    sustainConditionRow.getComboBox().addItem("Pedal up", 3);
    pitchSourceRow.getComboBox().addItem("Event note", 1);
    pitchSourceRow.getComboBox().addItem("Fixed root", 2);
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
    articulationRow.getComboBox().setExplicitFocusOrder(44);
    sampleSection.getDisclosureButton().setExplicitFocusOrder(45);
    velocityRangeRow.getLowSlider().setExplicitFocusOrder(46);
    velocityRangeRow.getHighSlider().setExplicitFocusOrder(47);
    roundRobinSection.getDisclosureButton().setExplicitFocusOrder(48);
    createRoundRobinPoolRow.getButton().setExplicitFocusOrder(49);
    addCompatibleZonesRow.getButton().setExplicitFocusOrder(50);
    normalizeRoundRobinPoolRow.getButton().setExplicitFocusOrder(51);
    removeRoundRobinPoolRow.getButton().setExplicitFocusOrder(52);
    mixSection.getDisclosureButton().setExplicitFocusOrder(53);
    gainRow.getSlider().setExplicitFocusOrder(54);
    panRow.getSlider().setExplicitFocusOrder(55);
    advancedSection.getDisclosureButton().setExplicitFocusOrder(56);
    loopToggleRow.getToggle().setExplicitFocusOrder(57);
    triggerModeRow.getComboBox().setExplicitFocusOrder(58);
    previewZoneRow.getButton().setExplicitFocusOrder(59);
    restoreRootKeyRow.getButton().setExplicitFocusOrder(60);
    performanceSection.getDisclosureButton().setExplicitFocusOrder(61);
    performanceEventRow.getComboBox().setExplicitFocusOrder(62);
    sustainConditionRow.getComboBox().setExplicitFocusOrder(63);
    pitchSourceRow.getComboBox().setExplicitFocusOrder(64);
    chokeGroupRow.getComboBox().setExplicitFocusOrder(65);
    chokeTargetRow.getComboBox().setExplicitFocusOrder(66);
    chokeFadeRow.getSlider().setExplicitFocusOrder(67);
    createChokeGroupRow.getButton().setExplicitFocusOrder(68);
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
    addOwnedRow(mapSectionContent, articulationRow, comboRowHeight);
    mapSectionContent.setSize(0, sliderRowHeight + 6 + rangeRowHeight + 6 + comboRowHeight);

    addOwnedRow(sampleSectionContent, velocityRangeRow, rangeRowHeight);
    sampleSectionContent.setSize(0, rangeRowHeight);

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
    addOwnedRow(performanceSectionContent, performanceEventRow, comboRowHeight);
    addOwnedRow(performanceSectionContent, sustainConditionRow, comboRowHeight);
    addOwnedRow(performanceSectionContent, pitchSourceRow, comboRowHeight);
    addOwnedRow(performanceSectionContent, chokeGroupRow, comboRowHeight);
    addOwnedRow(performanceSectionContent, chokeTargetRow, comboRowHeight);
    addOwnedRow(performanceSectionContent, chokeFadeRow, sliderRowHeight);
    addOwnedRow(performanceSectionContent, createChokeGroupRow, actionRowHeight);
    addOwnedRow(performanceSectionContent, performanceHintMessage, messageRowHeight);
    performanceSectionContent.setSize(0, comboRowHeight * 5 + sliderRowHeight + actionRowHeight
                                             + messageRowHeight + 7 * 6);

    mapSection.setContent(&mapSectionContent);
    sampleSection.setContent(&sampleSectionContent);
    mixSection.setContent(&mixSectionContent);
    advancedSection.setContent(&advancedSectionContent);
    performanceSection.setContent(&performanceSectionContent);
    mapSection.setOnExpandedChanged([this](bool) { resized(); });
    sampleSection.setOnExpandedChanged([this](bool) { resized(); });
    mixSection.setOnExpandedChanged([this](bool) { resized(); });
    advancedSection.setOnExpandedChanged([this](bool) { resized(); });
    performanceSection.setOnExpandedChanged([this](bool) { resized(); });

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
    performanceEventRow.getComboBox().onChange = [this] { commitCurrentValues("Update zone performance event"); };
    sustainConditionRow.getComboBox().onChange = [this] { commitCurrentValues("Update zone sustain condition"); };
    pitchSourceRow.getComboBox().onChange = [this] { commitCurrentValues("Update zone pitch source"); };
    chokeGroupRow.getComboBox().onChange = [this] { commitCurrentValues("Update choke group"); };
    chokeTargetRow.getComboBox().onChange = [this] { commitCurrentValues("Update choke target"); };
    bindCommitOnGestureFinished(chokeFadeRow.getSlider(), "Update choke fade");
    createChokeGroupRow.getButton().onClick = [this]
    {
        if (callbacks.onCreateChokeGroupRequested)
            callbacks.onCreateChokeGroupRequested();
    };
    articulationRow.getComboBox().onChange = [this]
    {
        if (callbacks.onArticulationCommitRequested && viewModel.hasSelection)
            callbacks.onArticulationCommitRequested(articulationRow.getComboBox().getText().toStdString());
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
    for (auto* component : {
             static_cast<juce::Component*>(&emptyStateMessage),
             static_cast<juce::Component*>(&mapSection),
             static_cast<juce::Component*>(&sampleSection),
             static_cast<juce::Component*>(&mixSection),
             static_cast<juce::Component*>(&advancedSection),
             static_cast<juce::Component*>(&performanceSection)
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
    layoutSection(mixSection, true);
    layoutSection(advancedSection, false);
    if (!area.isEmpty())
    {
        area.removeFromTop(sectionGap);
        layoutSection(performanceSection, false);
    }

    auto mapArea = mapSectionContent.getLocalBounds();
    rootKeyRow.setBounds(mapArea.removeFromTop(sliderRowHeight));
    mapArea.removeFromTop(6);
    keyRangeRow.setBounds(mapArea.removeFromTop(rangeRowHeight));
    mapArea.removeFromTop(6);
    articulationRow.setBounds(mapArea.removeFromTop(comboRowHeight));

    auto sampleArea = sampleSectionContent.getLocalBounds();
    velocityRangeRow.setBounds(sampleArea.removeFromTop(rangeRowHeight));

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

    auto performanceArea = performanceSectionContent.getLocalBounds();
    performanceEventRow.setBounds(performanceArea.removeFromTop(comboRowHeight)); performanceArea.removeFromTop(6);
    sustainConditionRow.setBounds(performanceArea.removeFromTop(comboRowHeight)); performanceArea.removeFromTop(6);
    pitchSourceRow.setBounds(performanceArea.removeFromTop(comboRowHeight)); performanceArea.removeFromTop(6);
    chokeGroupRow.setBounds(performanceArea.removeFromTop(comboRowHeight)); performanceArea.removeFromTop(6);
    chokeTargetRow.setBounds(performanceArea.removeFromTop(comboRowHeight)); performanceArea.removeFromTop(6);
    chokeFadeRow.setBounds(performanceArea.removeFromTop(sliderRowHeight)); performanceArea.removeFromTop(6);
    createChokeGroupRow.setBounds(performanceArea.removeFromTop(actionRowHeight)); performanceArea.removeFromTop(6);
    performanceHintMessage.setBounds(performanceArea.removeFromTop(messageRowHeight));
}

void ZoneMappingEditor::setViewModel(ZoneFieldValuesViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    emptyStateMessage.setText(juce::String::fromUTF8(viewModel.emptyStateText.c_str()));

    applyValuesToControls(viewModel);

    const auto hasSelection = viewModel.hasSelection;

    setVisibleAndAccessible(emptyStateMessage, !hasSelection);
    for (auto* component : {
             static_cast<juce::Component*>(&mapSection),
             static_cast<juce::Component*>(&sampleSection),
             static_cast<juce::Component*>(&mixSection),
             static_cast<juce::Component*>(&advancedSection),
             static_cast<juce::Component*>(&performanceSection),
             static_cast<juce::Component*>(&mapSectionContent),
             static_cast<juce::Component*>(&sampleSectionContent),
             static_cast<juce::Component*>(&mixSectionContent),
             static_cast<juce::Component*>(&advancedSectionContent),
             static_cast<juce::Component*>(&performanceSectionContent),
             static_cast<juce::Component*>(&rootKeyRow),
             static_cast<juce::Component*>(&keyRangeRow),
             static_cast<juce::Component*>(&articulationRow),
             static_cast<juce::Component*>(&velocityRangeRow),
             static_cast<juce::Component*>(&gainRow),
             static_cast<juce::Component*>(&panRow),
             static_cast<juce::Component*>(&loopToggleRow),
             static_cast<juce::Component*>(&triggerModeRow),
             static_cast<juce::Component*>(&performanceEventRow),
             static_cast<juce::Component*>(&sustainConditionRow),
             static_cast<juce::Component*>(&pitchSourceRow),
             static_cast<juce::Component*>(&chokeGroupRow),
             static_cast<juce::Component*>(&chokeTargetRow),
             static_cast<juce::Component*>(&chokeFadeRow),
             static_cast<juce::Component*>(&createChokeGroupRow),
             static_cast<juce::Component*>(&performanceHintMessage),
             static_cast<juce::Component*>(&previewZoneRow),
             static_cast<juce::Component*>(&restoreRootKeyRow),
             static_cast<juce::Component*>(&validationMessage),
             static_cast<juce::Component*>(&rootKeyRow.getSlider()),
             static_cast<juce::Component*>(&keyRangeRow.getLowSlider()),
             static_cast<juce::Component*>(&keyRangeRow.getHighSlider()),
             static_cast<juce::Component*>(&articulationRow.getComboBox()),
             static_cast<juce::Component*>(&velocityRangeRow.getLowSlider()),
             static_cast<juce::Component*>(&velocityRangeRow.getHighSlider()),
             static_cast<juce::Component*>(&gainRow.getSlider()),
             static_cast<juce::Component*>(&panRow.getSlider()),
             static_cast<juce::Component*>(&loopToggleRow.getToggle()),
             static_cast<juce::Component*>(&triggerModeRow.getComboBox()),
             static_cast<juce::Component*>(&performanceEventRow.getComboBox()),
             static_cast<juce::Component*>(&sustainConditionRow.getComboBox()),
             static_cast<juce::Component*>(&pitchSourceRow.getComboBox()),
             static_cast<juce::Component*>(&chokeGroupRow.getComboBox()),
             static_cast<juce::Component*>(&chokeTargetRow.getComboBox()),
             static_cast<juce::Component*>(&chokeFadeRow.getSlider()),
             static_cast<juce::Component*>(&createChokeGroupRow.getButton()),
             static_cast<juce::Component*>(&previewZoneRow.getButton()),
             static_cast<juce::Component*>(&restoreRootKeyRow.getButton())
         })
    {
        setVisibleAndAccessible(*component, hasSelection);
    }

    restoreRootKeyRow.getButton().setEnabled(viewModel.hasSelection);
    previewZoneRow.getButton().setEnabled(viewModel.hasSelection);
    previewZoneRow.getButton().setButtonText(viewModel.previewAdvancesRoundRobin
                                                 ? "Preview / Advance"
                                                 : "Preview Zone");
    previewZoneRow.getButton().setHelpText(viewModel.previewAdvancesRoundRobin
                                               ? "Auditions the selected zone and advances its active Round Robin slot."
                                               : "Auditions the selected zone from the mapping inspector.");
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
    const auto eventId = performanceEventRow.getComboBox().getSelectedId();
    values.performanceEvent = eventId == 2 ? drs::engine::PerformanceEventKind::noteOff
        : eventId == 3 ? drs::engine::PerformanceEventKind::release
        : eventId == 4 ? drs::engine::PerformanceEventKind::pedalDown
        : eventId == 5 ? drs::engine::PerformanceEventKind::pedalUp
                       : drs::engine::PerformanceEventKind::noteOn;
    const auto sustainId = sustainConditionRow.getComboBox().getSelectedId();
    values.performanceSustain = sustainId == 2 ? drs::engine::PerformanceSustainCondition::pedalDown
        : sustainId == 3 ? drs::engine::PerformanceSustainCondition::pedalUp
                         : drs::engine::PerformanceSustainCondition::any;
    values.performancePitchSource = pitchSourceRow.getComboBox().getSelectedId() == 2
        ? drs::engine::PerformancePitchSource::fixedRoot
        : drs::engine::PerformancePitchSource::eventNote;
    values.exclusiveGroupId = chokeGroupRow.getComboBox().getSelectedId() > 1
        ? chokeGroupRow.getComboBox().getText().toStdString() : std::string {};
    values.exclusiveTargetGroupId = chokeTargetRow.getComboBox().getSelectedId() > 1
        ? chokeTargetRow.getComboBox().getText().toStdString() : std::string {};
    values.chokeReleaseSeconds = chokeFadeRow.getSlider().getValue();

    if (values.performanceEvent == drs::engine::PerformanceEventKind::release)
    {
        values.performanceSustain = drs::engine::PerformanceSustainCondition::pedalUp;
        values.triggerMode = drs::engine::ZoneTriggerMode::oneShot;
    }
    else if (values.performanceEvent == drs::engine::PerformanceEventKind::pedalDown)
    {
        if (values.performanceSustain == drs::engine::PerformanceSustainCondition::pedalUp)
            values.performanceSustain = drs::engine::PerformanceSustainCondition::pedalDown;
        values.triggerMode = drs::engine::ZoneTriggerMode::oneShot;
    }
    else if (values.performanceEvent == drs::engine::PerformanceEventKind::pedalUp)
    {
        if (values.performanceSustain == drs::engine::PerformanceSustainCondition::pedalDown)
            values.performanceSustain = drs::engine::PerformanceSustainCondition::pedalUp;
        values.triggerMode = drs::engine::ZoneTriggerMode::oneShot;
    }
    if (values.performancePitchSource == drs::engine::PerformancePitchSource::fixedRoot
        && values.performanceEvent != drs::engine::PerformanceEventKind::pedalDown
        && values.performanceEvent != drs::engine::PerformanceEventKind::pedalUp)
        values.performancePitchSource = drs::engine::PerformancePitchSource::eventNote;

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
    articulationRow.getComboBox().clear(juce::dontSendNotification);
    int selectedArticulationId = 0;
    for (std::size_t index = 0; index < values.articulationIds.size(); ++index)
    {
        const auto itemId = static_cast<int>(index) + 1;
        articulationRow.getComboBox().addItem(juce::String::fromUTF8(values.articulationIds[index].c_str()), itemId);
        if (values.articulationIds[index] == values.articulationId)
            selectedArticulationId = itemId;
    }
    articulationRow.getComboBox().setSelectedId(selectedArticulationId > 0 ? selectedArticulationId : 1,
                                                 juce::dontSendNotification);
    articulationRow.getComboBox().setEnabled(values.hasSelection && !values.articulationIds.empty());
    articulationRow.getComboBox().setHelpText(values.hasMultipleZoneSelection
        ? "Assigns the selected articulation to every selected zone."
        : "Assigns the selected zone to an articulation.");
    velocityRangeRow.getLowSlider().setHelpText(values.hasMultipleZoneSelection
        ? "Sets the low velocity for every selected zone."
        : "Sets the selected zone's low velocity.");
    velocityRangeRow.getHighSlider().setHelpText(values.hasMultipleZoneSelection
        ? "Sets the high velocity for every selected zone."
        : "Sets the selected zone's high velocity.");
    gainRow.getSlider().setValue(values.gainDb, juce::dontSendNotification);
    panRow.getSlider().setValue(values.pan, juce::dontSendNotification);
    loopToggleRow.getToggle().setToggleState(values.loopEnabled, juce::dontSendNotification);
    triggerModeRow.getComboBox().setSelectedId(
        values.triggerMode == drs::engine::ZoneTriggerMode::oneShot ? 2 : 1,
        juce::dontSendNotification);
    const auto eventId = values.performanceEvent == drs::engine::PerformanceEventKind::noteOff ? 2
        : values.performanceEvent == drs::engine::PerformanceEventKind::release ? 3
        : values.performanceEvent == drs::engine::PerformanceEventKind::pedalDown ? 4
        : values.performanceEvent == drs::engine::PerformanceEventKind::pedalUp ? 5 : 1;
    performanceEventRow.getComboBox().setSelectedId(eventId, juce::dontSendNotification);
    sustainConditionRow.getComboBox().setSelectedId(
        values.performanceSustain == drs::engine::PerformanceSustainCondition::pedalDown ? 2
        : values.performanceSustain == drs::engine::PerformanceSustainCondition::pedalUp ? 3 : 1,
        juce::dontSendNotification);
    const auto pedalEvent = values.performanceEvent == drs::engine::PerformanceEventKind::pedalDown
        || values.performanceEvent == drs::engine::PerformanceEventKind::pedalUp;
    pitchSourceRow.getComboBox().setSelectedId(
        values.performancePitchSource == drs::engine::PerformancePitchSource::fixedRoot ? 2 : 1,
        juce::dontSendNotification);
    pitchSourceRow.getComboBox().setItemEnabled(2, pedalEvent);
    chokeGroupRow.getComboBox().clear(juce::dontSendNotification);
    chokeGroupRow.getComboBox().addItem("(none)", 1);
    chokeTargetRow.getComboBox().clear(juce::dontSendNotification);
    chokeTargetRow.getComboBox().addItem("(none)", 1);
    int groupId = 1;
    int targetId = 1;
    for (std::size_t index = 0; index < values.exclusiveGroupIds.size(); ++index)
    {
        const auto itemId = static_cast<int>(index) + 2;
        const auto& group = values.exclusiveGroupIds[index];
        chokeGroupRow.getComboBox().addItem(juce::String::fromUTF8(group.c_str()), itemId);
        if (group == values.exclusiveGroupId) groupId = itemId;
        if (group != values.exclusiveGroupId)
        {
            chokeTargetRow.getComboBox().addItem(juce::String::fromUTF8(group.c_str()), itemId);
            if (group == values.exclusiveTargetGroupId) targetId = itemId;
        }
    }
    chokeGroupRow.getComboBox().setSelectedId(groupId, juce::dontSendNotification);
    chokeTargetRow.getComboBox().setSelectedId(targetId, juce::dontSendNotification);
    chokeFadeRow.getSlider().setValue(values.chokeReleaseSeconds, juce::dontSendNotification);
    const auto eventNeedsOneShot = values.performanceEvent == drs::engine::PerformanceEventKind::release || pedalEvent;
    triggerModeRow.getComboBox().setItemEnabled(1, !eventNeedsOneShot);
    sustainConditionRow.getComboBox().setItemEnabled(2, values.performanceEvent != drs::engine::PerformanceEventKind::pedalUp
                                                          && values.performanceEvent != drs::engine::PerformanceEventKind::release);
    sustainConditionRow.getComboBox().setItemEnabled(3, values.performanceEvent != drs::engine::PerformanceEventKind::pedalDown);
    performanceHintMessage.setText(eventNeedsOneShot
        ? "This event uses one-shot playback; incompatible pedal conditions are unavailable."
        : "Event note is used for note-on, note-off, and effective-release routes. Fixed root is available for pedal routes.");
}

void ZoneMappingEditor::refreshValidationMessage(const juce::String& messageText)
{
    validationMessage.setText(messageText.isEmpty()
                                  ? "Ranges are normalized on commit to keep low and high values valid."
                                  : messageText);
}
} // namespace drs::app::authoring
