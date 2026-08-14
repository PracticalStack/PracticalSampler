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
      crossfadeSection("Velocity Crossfades", "authoringVelocityCrossfadeInspectorSection", false),
      roundRobinSection("Round Robin", "authoringRoundRobinInspectorSection", false),
      mixSection("Mix", "authoringMixInspectorSection", false),
      advancedSection("Advanced", "authoringAdvancedInspectorSection", false),
      performanceSection("Performance", "authoringPerformanceInspectorSection", false),
      rootKeyRow("Root Key", "authoringRootKeyRow", 0, 127, 1),
      keyRangeRow("Key Range", "authoringKeyRangeRow", "Low", "High", 0, 127, 1),
      articulationRow("Articulation", "authoringZoneArticulationRow"),
      velocityRangeRow("Velocity Range", "authoringVelocityRangeRow", "Low", "High", 1, 127, 1),
      crossfadeFadeInMessage("authoringCrossfadeFadeInStatus", juce::Justification::centredLeft),
      crossfadeFadeOutMessage("authoringCrossfadeFadeOutStatus", juce::Justification::centredLeft),
      crossfadeOverlapRow("Overlap", "authoringCrossfadeOverlapRow", "Low", "High", 1, 127, 1),
      createCrossfadeRow("Relationship", "authoringCreateCrossfadeRow", "Create Crossfade"),
      updateCrossfadeRow("Overlap", "authoringUpdateCrossfadeRow", "Apply Overlap"),
      removeCrossfadeRow("Relationship", "authoringRemoveCrossfadeRow", "Remove Crossfade"),
      createCrossfadeStackRow("Layer Stack", "authoringCreateCrossfadeStackRow", "Create Stack Crossfades"),
      removeCrossfadeStackRow("Layer Stack", "authoringRemoveCrossfadeStackRow", "Remove Stack Crossfades"),
      auditionCrossfadeRow("Audition", "authoringCrossfadeAuditionRow", "Audition 5 Steps"),
      crossfadeAuditionMessage("authoringCrossfadeAuditionStatus", juce::Justification::centredLeft),
      crossfadeGuidanceMessage("authoringCrossfadeGuidance", juce::Justification::centredLeft),
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
      releaseRow("Release (s)", "authoringReleaseRow", 0.0, 30.0, 0.01),
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
    crossfadeOverlapRow.getLowSlider().setComponentID("authoringCrossfadeOverlapLowSlider");
    crossfadeOverlapRow.getHighSlider().setComponentID("authoringCrossfadeOverlapHighSlider");
    createCrossfadeRow.getButton().setComponentID("authoringCreateCrossfadeButton");
    updateCrossfadeRow.getButton().setComponentID("authoringUpdateCrossfadeButton");
    removeCrossfadeRow.getButton().setComponentID("authoringRemoveCrossfadeButton");
    createCrossfadeStackRow.getButton().setComponentID("authoringCreateCrossfadeStackButton");
    removeCrossfadeStackRow.getButton().setComponentID("authoringRemoveCrossfadeStackButton");
    auditionCrossfadeRow.getButton().setComponentID("authoringAuditionCrossfadeButton");
    gainRow.getSlider().setComponentID("authoringGainSlider");
    panRow.getSlider().setComponentID("authoringPanSlider");
    loopToggleRow.getToggle().setComponentID("authoringLoopEnabledToggle");
    releaseRow.getSlider().setComponentID("authoringReleaseSecondsSlider");
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
    pitchSourceRow.getComboBox().addItem("Event key / fixed pitch", 3);
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
    crossfadeSection.getDisclosureButton().setExplicitFocusOrder(48);
    crossfadeOverlapRow.getLowSlider().setExplicitFocusOrder(49);
    crossfadeOverlapRow.getHighSlider().setExplicitFocusOrder(50);
    createCrossfadeRow.getButton().setExplicitFocusOrder(51);
    updateCrossfadeRow.getButton().setExplicitFocusOrder(52);
    removeCrossfadeRow.getButton().setExplicitFocusOrder(53);
    roundRobinSection.getDisclosureButton().setExplicitFocusOrder(54);
    createRoundRobinPoolRow.getButton().setExplicitFocusOrder(55);
    addCompatibleZonesRow.getButton().setExplicitFocusOrder(56);
    normalizeRoundRobinPoolRow.getButton().setExplicitFocusOrder(57);
    removeRoundRobinPoolRow.getButton().setExplicitFocusOrder(58);
    mixSection.getDisclosureButton().setExplicitFocusOrder(59);
    gainRow.getSlider().setExplicitFocusOrder(60);
    panRow.getSlider().setExplicitFocusOrder(61);
    advancedSection.getDisclosureButton().setExplicitFocusOrder(62);
    loopToggleRow.getToggle().setExplicitFocusOrder(63);
    releaseRow.getSlider().setExplicitFocusOrder(64);
    triggerModeRow.getComboBox().setExplicitFocusOrder(65);
    previewZoneRow.getButton().setExplicitFocusOrder(66);
    restoreRootKeyRow.getButton().setExplicitFocusOrder(67);
    performanceSection.getDisclosureButton().setExplicitFocusOrder(68);
    performanceEventRow.getComboBox().setExplicitFocusOrder(69);
    sustainConditionRow.getComboBox().setExplicitFocusOrder(70);
    pitchSourceRow.getComboBox().setExplicitFocusOrder(71);
    chokeGroupRow.getComboBox().setExplicitFocusOrder(72);
    chokeTargetRow.getComboBox().setExplicitFocusOrder(73);
    chokeFadeRow.getSlider().setExplicitFocusOrder(74);
    createChokeGroupRow.getButton().setExplicitFocusOrder(75);
    releaseRow.getSlider().setHelpText(
        "Sets the selected zone's amplitude release time after note-off, in seconds.");
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
    crossfadeOverlapRow.getLowSlider().setHelpText(
        "Sets the low MIDI velocity of the shared linear crossfade overlap. Changes commit only when applied.");
    crossfadeOverlapRow.getHighSlider().setHelpText(
        "Sets the high MIDI velocity of the shared linear crossfade overlap. Changes commit only when applied.");
    createCrossfadeRow.getButton().setHelpText(
        "Creates one atomic linear velocity crossfade between the two selected compatible layers.");
    updateCrossfadeRow.getButton().setHelpText(
        "Applies the displayed overlap to both sides of the selected crossfade relationship.");
    removeCrossfadeRow.getButton().setHelpText(
        "Removes both sides of the selected crossfade relationship without changing other ranges.");
    createCrossfadeStackRow.getButton().setHelpText(
        "Creates all adjacent crossfades for the selected compatible velocity-layer stack in one undo step.");
    removeCrossfadeStackRow.getButton().setHelpText(
        "Removes crossfade descriptors across the selected stack without restoring or changing velocity ranges.");
    auditionCrossfadeRow.getButton().setHelpText(
        "Auditions below, at both edges, at the midpoint, and above the selected crossfade using draft playback.");

    addOwnedRow(mapSectionContent, rootKeyRow, sliderRowHeight);
    addOwnedRow(mapSectionContent, keyRangeRow, rangeRowHeight);
    addOwnedRow(mapSectionContent, articulationRow, comboRowHeight);
    mapSectionContent.setSize(0, sliderRowHeight + 6 + rangeRowHeight + 6 + comboRowHeight);

    addOwnedRow(sampleSectionContent, velocityRangeRow, rangeRowHeight);
    addOwnedRow(crossfadeSectionContent, crossfadeFadeInMessage, messageRowHeight);
    addOwnedRow(crossfadeSectionContent, crossfadeFadeOutMessage, messageRowHeight);
    addOwnedRow(crossfadeSectionContent, crossfadeOverlapRow, rangeRowHeight);
    addOwnedRow(crossfadeSectionContent, createCrossfadeRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, updateCrossfadeRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, removeCrossfadeRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, createCrossfadeStackRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, removeCrossfadeStackRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, auditionCrossfadeRow, actionRowHeight);
    addOwnedRow(crossfadeSectionContent, crossfadeAuditionMessage, messageRowHeight);
    addOwnedRow(crossfadeSectionContent, crossfadeGuidanceMessage, messageRowHeight);
    crossfadeSectionContent.setSize(0, messageRowHeight * 4 + rangeRowHeight + actionRowHeight * 6 + 10 * 6);
    crossfadeSection.setContent(&crossfadeSectionContent);
    sampleSectionContent.addAndMakeVisible(crossfadeSection);
    updateSampleSectionContentHeight();

    addOwnedRow(mixSectionContent, gainRow, sliderRowHeight);
    addOwnedRow(mixSectionContent, panRow, sliderRowHeight);
    mixSectionContent.setSize(0, sliderRowHeight + 6 + sliderRowHeight);

    addOwnedRow(advancedSectionContent, loopToggleRow, toggleRowHeight);
    addOwnedRow(advancedSectionContent, releaseRow, sliderRowHeight);
    addOwnedRow(advancedSectionContent, triggerModeRow, comboRowHeight);
    addOwnedRow(advancedSectionContent, previewZoneRow, actionRowHeight);
    addOwnedRow(advancedSectionContent, restoreRootKeyRow, actionRowHeight);
    addOwnedRow(advancedSectionContent, validationMessage, messageRowHeight);
    advancedSectionContent.setSize(0, toggleRowHeight + 6 + sliderRowHeight + 6 + comboRowHeight + 6 + actionRowHeight + 6
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
    mixSection.setContent(&mixSectionContent);
    advancedSection.setContent(&advancedSectionContent);
    performanceSection.setContent(&performanceSectionContent);
    mapSection.setOnExpandedChanged([this](bool) { resized(); });
    sampleSection.setOnExpandedChanged([this](bool) { resized(); });
    crossfadeSection.setOnExpandedChanged([this](bool)
    {
        updateSampleSectionContentHeight();
        resized();
    });
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
    bindCommitOnGestureFinished(releaseRow.getSlider(), "Update zone release");

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
    createCrossfadeRow.getButton().onClick = [this] { invokeCrossfadeAction(true); };
    updateCrossfadeRow.getButton().onClick = [this] { invokeCrossfadeAction(false); };
    removeCrossfadeRow.getButton().onClick = [this]
    {
        if (callbacks.onRemoveVelocityCrossfadeRequested && viewModel.crossfadeCanRemove)
            callbacks.onRemoveVelocityCrossfadeRequested(viewModel.crossfadeLowerZoneId,
                                                         viewModel.crossfadeUpperZoneId);
    };
    createCrossfadeStackRow.getButton().onClick = [this] { invokeCrossfadeStackAction(true); };
    removeCrossfadeStackRow.getButton().onClick = [this] { invokeCrossfadeStackAction(false); };
    auditionCrossfadeRow.getButton().onClick = [this]
    {
        if (viewModel.crossfadeCanAudition && callbacks.onAuditionVelocityCrossfadeRequested)
            callbacks.onAuditionVelocityCrossfadeRequested(viewModel.crossfadeAuditionVelocities);
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
    sampleArea.removeFromTop(6);
    crossfadeSection.setBounds(sampleArea.removeFromTop(crossfadeSection.getPreferredHeight()));

    auto crossfadeArea = crossfadeSectionContent.getLocalBounds();
    crossfadeFadeInMessage.setBounds(crossfadeArea.removeFromTop(messageRowHeight)); crossfadeArea.removeFromTop(6);
    crossfadeFadeOutMessage.setBounds(crossfadeArea.removeFromTop(messageRowHeight)); crossfadeArea.removeFromTop(6);
    crossfadeOverlapRow.setBounds(crossfadeArea.removeFromTop(rangeRowHeight)); crossfadeArea.removeFromTop(6);
    createCrossfadeRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    updateCrossfadeRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    removeCrossfadeRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    createCrossfadeStackRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    removeCrossfadeStackRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    auditionCrossfadeRow.setBounds(crossfadeArea.removeFromTop(actionRowHeight)); crossfadeArea.removeFromTop(6);
    crossfadeAuditionMessage.setBounds(crossfadeArea.removeFromTop(messageRowHeight)); crossfadeArea.removeFromTop(6);
    crossfadeGuidanceMessage.setBounds(crossfadeArea.removeFromTop(messageRowHeight));

    auto mixArea = mixSectionContent.getLocalBounds();
    gainRow.setBounds(mixArea.removeFromTop(sliderRowHeight));
    mixArea.removeFromTop(6);
    panRow.setBounds(mixArea.removeFromTop(sliderRowHeight));

    auto advancedArea = advancedSectionContent.getLocalBounds();
    loopToggleRow.setBounds(advancedArea.removeFromTop(toggleRowHeight));
    advancedArea.removeFromTop(6);
    releaseRow.setBounds(advancedArea.removeFromTop(sliderRowHeight));
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
             static_cast<juce::Component*>(&crossfadeSection),
             static_cast<juce::Component*>(&mixSection),
             static_cast<juce::Component*>(&advancedSection),
             static_cast<juce::Component*>(&performanceSection),
             static_cast<juce::Component*>(&mapSectionContent),
             static_cast<juce::Component*>(&sampleSectionContent),
             static_cast<juce::Component*>(&crossfadeSectionContent),
             static_cast<juce::Component*>(&mixSectionContent),
             static_cast<juce::Component*>(&advancedSectionContent),
             static_cast<juce::Component*>(&performanceSectionContent),
             static_cast<juce::Component*>(&rootKeyRow),
             static_cast<juce::Component*>(&keyRangeRow),
             static_cast<juce::Component*>(&articulationRow),
             static_cast<juce::Component*>(&velocityRangeRow),
             static_cast<juce::Component*>(&crossfadeFadeInMessage),
             static_cast<juce::Component*>(&crossfadeFadeOutMessage),
             static_cast<juce::Component*>(&crossfadeOverlapRow),
             static_cast<juce::Component*>(&createCrossfadeRow),
             static_cast<juce::Component*>(&updateCrossfadeRow),
             static_cast<juce::Component*>(&removeCrossfadeRow),
             static_cast<juce::Component*>(&createCrossfadeStackRow),
             static_cast<juce::Component*>(&removeCrossfadeStackRow),
             static_cast<juce::Component*>(&auditionCrossfadeRow),
             static_cast<juce::Component*>(&crossfadeAuditionMessage),
             static_cast<juce::Component*>(&crossfadeGuidanceMessage),
             static_cast<juce::Component*>(&gainRow),
             static_cast<juce::Component*>(&panRow),
             static_cast<juce::Component*>(&loopToggleRow),
             static_cast<juce::Component*>(&releaseRow),
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
             static_cast<juce::Component*>(&crossfadeOverlapRow.getLowSlider()),
             static_cast<juce::Component*>(&crossfadeOverlapRow.getHighSlider()),
             static_cast<juce::Component*>(&createCrossfadeRow.getButton()),
             static_cast<juce::Component*>(&updateCrossfadeRow.getButton()),
             static_cast<juce::Component*>(&removeCrossfadeRow.getButton()),
             static_cast<juce::Component*>(&createCrossfadeStackRow.getButton()),
             static_cast<juce::Component*>(&removeCrossfadeStackRow.getButton()),
             static_cast<juce::Component*>(&auditionCrossfadeRow.getButton()),
             static_cast<juce::Component*>(&gainRow.getSlider()),
             static_cast<juce::Component*>(&panRow.getSlider()),
             static_cast<juce::Component*>(&loopToggleRow.getToggle()),
             static_cast<juce::Component*>(&releaseRow.getSlider()),
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
    values.releaseSeconds = releaseRow.getSlider().getValue();
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
    const auto pitchSourceId = pitchSourceRow.getComboBox().getSelectedId();
    values.performancePitchSource = pitchSourceId == 2
        ? drs::engine::PerformancePitchSource::fixedRoot
        : pitchSourceId == 3
            ? drs::engine::PerformancePitchSource::eventKeyFixedPitch
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
    crossfadeFadeInMessage.setText(juce::String::fromUTF8(values.crossfadeFadeInText.c_str()));
    crossfadeFadeOutMessage.setText(juce::String::fromUTF8(values.crossfadeFadeOutText.c_str()));
    crossfadeOverlapRow.getLowSlider().setValue(values.crossfadeOverlapLow, juce::dontSendNotification);
    crossfadeOverlapRow.getHighSlider().setValue(values.crossfadeOverlapHigh, juce::dontSendNotification);
    crossfadeGuidanceMessage.setText(juce::String::fromUTF8(values.crossfadeGuidanceText.c_str()));
    crossfadeAuditionMessage.setText(juce::String::fromUTF8(values.crossfadeAuditionText.c_str()));
    createCrossfadeRow.getButton().setEnabled(values.crossfadeCanCreate);
    updateCrossfadeRow.getButton().setEnabled(values.crossfadeCanEdit);
    removeCrossfadeRow.getButton().setEnabled(values.crossfadeCanRemove);
    createCrossfadeStackRow.getButton().setEnabled(values.crossfadeCanCreateStack);
    removeCrossfadeStackRow.getButton().setEnabled(values.crossfadeCanRemoveStack);
    auditionCrossfadeRow.getButton().setEnabled(values.crossfadeCanAudition);
    createCrossfadeRow.getButton().setButtonText("Create Crossfade");
    updateCrossfadeRow.getButton().setButtonText("Apply Overlap");
    removeCrossfadeRow.getButton().setButtonText("Remove Crossfade");
    createCrossfadeStackRow.getButton().setButtonText("Create Stack Crossfades");
    removeCrossfadeStackRow.getButton().setButtonText("Remove Stack Crossfades");
    auditionCrossfadeRow.getButton().setButtonText("Audition 5 Steps");
    gainRow.getSlider().setValue(values.gainDb, juce::dontSendNotification);
    panRow.getSlider().setValue(values.pan, juce::dontSendNotification);
    loopToggleRow.getToggle().setToggleState(values.loopEnabled, juce::dontSendNotification);
    auto& releaseSlider = releaseRow.getSlider();
    if (values.releaseSeconds > releaseSlider.getMaximum())
        releaseSlider.setRange(0.0, values.releaseSeconds * 2.0, 0.01);
    releaseSlider.setValue(values.releaseSeconds, juce::dontSendNotification);
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
        values.performancePitchSource == drs::engine::PerformancePitchSource::fixedRoot ? 2
        : values.performancePitchSource == drs::engine::PerformancePitchSource::eventKeyFixedPitch ? 3 : 1,
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

void ZoneMappingEditor::updateSampleSectionContentHeight()
{
    sampleSectionContent.setSize(0, rangeRowHeight + 6 + crossfadeSection.getPreferredHeight());
    sampleSection.setContent(&sampleSectionContent);
}

void ZoneMappingEditor::invokeCrossfadeAction(const bool create)
{
    if (!viewModel.hasSelection || (create ? !viewModel.crossfadeCanCreate : !viewModel.crossfadeCanEdit))
        return;

    auto low = static_cast<int>(crossfadeOverlapRow.getLowSlider().getValue());
    auto high = static_cast<int>(crossfadeOverlapRow.getHighSlider().getValue());
    if (low > high)
    {
        std::swap(low, high);
        crossfadeOverlapRow.getLowSlider().setValue(low, juce::dontSendNotification);
        crossfadeOverlapRow.getHighSlider().setValue(high, juce::dontSendNotification);
        crossfadeGuidanceMessage.setText("Crossfade overlap was normalized to keep Low < High.");
    }
    if (low >= high)
    {
        crossfadeGuidanceMessage.setText("Crossfade overlap needs at least two MIDI velocity values.");
        return;
    }

    if (create && callbacks.onCreateVelocityCrossfadeRequested)
        callbacks.onCreateVelocityCrossfadeRequested(viewModel.crossfadeLowerZoneId,
                                                     viewModel.crossfadeUpperZoneId,
                                                     low, high);
    else if (!create && callbacks.onUpdateVelocityCrossfadeRequested)
        callbacks.onUpdateVelocityCrossfadeRequested(viewModel.crossfadeLowerZoneId,
                                                     viewModel.crossfadeUpperZoneId,
                                                     low, high);
}

void ZoneMappingEditor::invokeCrossfadeStackAction(const bool create)
{
    if (!viewModel.hasSelection
        || (create ? !viewModel.crossfadeCanCreateStack : !viewModel.crossfadeCanRemoveStack))
        return;
    if (create)
    {
        const auto low = static_cast<int>(crossfadeOverlapRow.getLowSlider().getValue());
        const auto high = static_cast<int>(crossfadeOverlapRow.getHighSlider().getValue());
        if (high <= low)
        {
            crossfadeGuidanceMessage.setText("Stack overlap needs at least two MIDI velocity values.");
            return;
        }
        if (callbacks.onCreateVelocityCrossfadeStackRequested)
            callbacks.onCreateVelocityCrossfadeStackRequested(viewModel.crossfadeStackZoneIds, high - low);
    }
    else if (callbacks.onRemoveVelocityCrossfadeStackRequested)
    {
        callbacks.onRemoveVelocityCrossfadeStackRequested(viewModel.crossfadeStackZoneIds);
    }
}

void ZoneMappingEditor::refreshValidationMessage(const juce::String& messageText)
{
    validationMessage.setText(messageText.isEmpty()
                                  ? "Ranges are normalized on commit to keep low and high values valid."
                                  : messageText);
}
} // namespace drs::app::authoring
