#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/AuthoringPreviewModel.h"
#include "shared/authoring/AuthoringSummaryStrip.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/ZoneMappingEditor.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}

void requireComponentVisibleWithin(juce::Component& root,
                                   const juce::String& componentId,
                                   const juce::Rectangle<int>& containerBounds)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID: " + componentId.toStdString());
    require(component->isVisible(), "Component should be visible: " + componentId.toStdString());
    require(!component->getBounds().isEmpty(), "Component bounds should not be empty: " + componentId.toStdString());
    require(containerBounds.contains(component->getBounds()),
            "Component should remain inside the panel bounds: " + componentId.toStdString());
}

void requireComponentVisible(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID: " + componentId.toStdString());
    require(component->isVisible(), "Component should be visible: " + componentId.toStdString());
    require(!component->getBounds().isEmpty(), "Component bounds should not be empty: " + componentId.toStdString());
}

juce::Button& requireButton(juce::Component& root, const juce::String& componentId)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, componentId));
    require(button != nullptr, "Missing button ID: " + componentId.toStdString());
    return *button;
}

juce::Slider& requireSlider(juce::Component& root, const juce::String& componentId)
{
    auto* slider = dynamic_cast<juce::Slider*>(findDescendantById(root, componentId));
    require(slider != nullptr, "Missing slider ID: " + componentId.toStdString());
    return *slider;
}

juce::Label& requireSliderTextBoxLabel(juce::Slider& slider)
{
    for (int childIndex = 0; childIndex < slider.getNumChildComponents(); ++childIndex)
    {
        if (auto* label = dynamic_cast<juce::Label*>(slider.getChildComponent(childIndex)))
            return *label;
    }

    throw std::runtime_error("Slider is missing its text-box label child.");
}

std::string requireMessageText(juce::Component& root, const juce::String& componentId)
{
    auto* message = findDescendantById(root, componentId);
    require(message != nullptr, "Missing message ID: " + componentId.toStdString());

    for (int childIndex = 0; childIndex < message->getNumChildComponents(); ++childIndex)
    {
        if (auto* label = dynamic_cast<juce::Label*>(message->getChildComponent(childIndex)))
            return label->getText().toStdString();
    }

    throw std::runtime_error("Message component is missing its label child.");
}

juce::ToggleButton& requireToggleButton(juce::Component& root, const juce::String& componentId)
{
    auto* toggle = dynamic_cast<juce::ToggleButton*>(findDescendantById(root, componentId));
    require(toggle != nullptr, "Missing toggle ID: " + componentId.toStdString());
    return *toggle;
}

juce::ComboBox& requireComboBox(juce::Component& root, const juce::String& componentId)
{
    auto* combo = dynamic_cast<juce::ComboBox*>(findDescendantById(root, componentId));
    require(combo != nullptr, "Missing combo box ID: " + componentId.toStdString());
    return *combo;
}

drs::app::authoring::ZoneMapCanvas& requireZoneMapCanvas(juce::Component& root, const juce::String& componentId)
{
    auto* zoneMap = dynamic_cast<drs::app::authoring::ZoneMapCanvas*>(findDescendantById(root, componentId));
    require(zoneMap != nullptr, "Missing zone map ID: " + componentId.toStdString());
    return *zoneMap;
}

int countDescendantsById(juce::Component& root, const juce::String& componentId)
{
    auto count = root.getComponentID() == componentId ? 1 : 0;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
        count += countDescendantsById(*root.getChildComponent(index), componentId);

    return count;
}

bool componentTreeContainsLabelText(juce::Component& root, const juce::String& expectedText)
{
    if (auto* label = dynamic_cast<juce::Label*>(&root))
    {
        if (label->getText() == expectedText)
            return true;
    }

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (componentTreeContainsLabelText(*root.getChildComponent(index), expectedText))
            return true;
    }

    return false;
}

juce::Point<float> computeZoneMapPoint(const juce::Component& zoneMap,
                                       const drs::engine::AuthoringZoneSummary& zone)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
    const auto width = std::max(10.0f,
                                inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
    const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
    const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));
    return {x + (width * 0.5f), y + (height * 0.5f)};
}

drs::engine::AuthoringZoneSummary makeZoneSummary(const drs::engine::RuntimeProjectZoneDefinition& zone)
{
    drs::engine::AuthoringZoneSummary summary;
    summary.id = zone.id;
    summary.displayName = zone.displayName;
    summary.sampleSourceId = zone.sampleSourceId;
    summary.articulationId = zone.articulationId;
    summary.rootKey = zone.rootKey;
    summary.keyLow = zone.keyLow;
    summary.keyHigh = zone.keyHigh;
    summary.velocityLow = zone.velocityLow;
    summary.velocityHigh = zone.velocityHigh;
    summary.gainDb = zone.gainDb;
    summary.pan = zone.pan;
    summary.loopEnabled = zone.loopEnabled;
    summary.selected = true;
    return summary;
}

juce::Point<float> computeZoneMapHandlePoint(const juce::Component& zoneMap,
                                             const drs::engine::AuthoringZoneSummary& zone,
                                             drs::app::authoring::ZoneMapCanvas::RangeHandle handle)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
    const auto width = std::max(10.0f,
                                inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
    const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
    const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));
    const auto bounds = juce::Rectangle<float>(x, y, width, height);

    switch (handle)
    {
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::keyLow:
            return {bounds.getX(), bounds.getCentreY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::keyHigh:
            return {bounds.getRight(), bounds.getCentreY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityHigh:
            return {bounds.getCentreX(), bounds.getY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityLow:
            return {bounds.getCentreX(), bounds.getBottom()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::none:
        default:
            return bounds.getCentre();
    }
}

juce::Point<float> computeZoneMapTargetPointForKey(const juce::Component& zoneMap,
                                                   const drs::engine::AuthoringZoneSummary& zone,
                                                   int midiKey)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto normalizedKey = juce::jlimit(0.0f, 127.0f, static_cast<float>(midiKey)) / 127.0f;
    const auto x = inner.getX() + inner.getWidth() * normalizedKey;
    return {x, computeZoneMapPoint(zoneMap, zone).y};
}

juce::Point<float> computeZoneMapTargetPointForVelocity(const juce::Component& zoneMap,
                                                        const drs::engine::AuthoringZoneSummary& zone,
                                                        int velocity)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto clampedVelocity = juce::jlimit(1.0f, 127.0f, static_cast<float>(velocity));
    const auto normalizedVelocity = 1.0f - (clampedVelocity / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocity;
    return {computeZoneMapPoint(zoneMap, zone).x, y};
}

std::string describeBounds(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    if (component == nullptr)
        return componentId.toStdString() + ": missing";

    const auto bounds = component->getBounds();
    return componentId.toStdString()
        + ": x=" + std::to_string(bounds.getX())
        + ", y=" + std::to_string(bounds.getY())
        + ", w=" + std::to_string(bounds.getWidth())
        + ", h=" + std::to_string(bounds.getHeight())
        + ", visible=" + std::string(component->isVisible() ? "true" : "false");
}

drs::app::AuthoringWaveformPreview makePreviewFixture()
{
    drs::app::AuthoringWaveformPreview preview;
    preview.available = true;
    preview.state = "ready";
    preview.sourcePath = "fixtures/phase2/lead-a4-sustain.wav";
    preview.formatName = "WAV";
    preview.durationSeconds = 2.418;
    preview.sampleRate = 48000.0;
    preview.frameCount = 116064;
    preview.channelCount = 2;
    preview.loopEnabled = true;
    preview.loopStartFrame = 12000;
    preview.loopEndFrame = 88000;

    preview.points.reserve(96);
    for (int index = 0; index < 96; ++index)
    {
        const auto phase = static_cast<float>(index % 16) / 15.0f;
        const auto amplitude = 0.2f + static_cast<float>(index % 5) * 0.12f;
        preview.points.push_back({-amplitude * (0.3f + phase * 0.7f), amplitude});
    }

    return preview;
}

drs::app::AuthoringImportResponsivenessSnapshot makeImportMetricsFixture()
{
    drs::app::AuthoringImportResponsivenessSnapshot metrics;
    metrics.available = true;
    metrics.state = "ready";
    metrics.totalItemCount = 24;
    metrics.processedCount = 24;
    metrics.warningItemCount = 1;
    metrics.failedItemCount = 0;
    metrics.lastProcessDurationMicros = 410;
    metrics.averageProcessDurationMicros = 320;
    metrics.maxProcessDurationMicros = 870;
    metrics.lastProcessedItemId = "lead-a4-sustain";
    return metrics;
}

void saveComponentPng(juce::Component& component, const fs::path& path)
{
    juce::Image image(juce::Image::ARGB, component.getWidth(), component.getHeight(), true);
    juce::Graphics graphics(image);
    component.paintEntireComponent(graphics, true);

    juce::PNGImageFormat pngFormat;
    juce::File targetFile(path.string());
    targetFile.getParentDirectory().createDirectory();
    juce::FileOutputStream output(targetFile);
    require(output.openedOk(), "Could not create diagnostic image: " + path.string());
    require(pngFormat.writeImageToStream(image, output), "Could not write diagnostic image: " + path.string());
}

void exerciseSummaryStripLeaf(const fs::path& outputDirectory)
{
    drs::app::authoring::AuthoringSummaryStrip strip;

    int previewRequests = 0;
    int undoRequests = 0;
    int redoRequests = 0;
    int saveRequests = 0;
    drs::app::authoring::SelectionSummaryCallbacks callbacks;
    callbacks.onPreviewRequested = [&previewRequests] { ++previewRequests; };
    callbacks.onUndoRequested = [&undoRequests] { ++undoRequests; };
    callbacks.onRedoRequested = [&redoRequests] { ++redoRequests; };
    callbacks.onMarkSavedRequested = [&saveRequests] { ++saveRequests; };
    strip.setCallbacks(std::move(callbacks));

    drs::app::authoring::SelectionSummaryViewModel viewModel;
    viewModel.title = "Lead Sustain";
    viewModel.statusText = "Selected zone is ready to preview";
    viewModel.sourceText = "Source: fixtures/phase2/lead-a4-sustain.wav";
    viewModel.articulationText = "Articulation: sustain";
    viewModel.canPreview = true;
    viewModel.canUndo = true;
    viewModel.canRedo = true;
    viewModel.dirty = true;

    strip.setViewModel(viewModel);
    strip.setTopLeftPosition(0, 0);
    strip.setSize(760, drs::app::authoring::heroHeight);
    strip.setVisible(true);
    strip.resized();

    const auto bounds = strip.getLocalBounds();
    requireComponentVisibleWithin(strip, "authoringSummaryStrip", bounds);
    requireComponentVisibleWithin(strip, "authoringPreviewButton", bounds);
    requireComponentVisibleWithin(strip, "authoringUndoButton", bounds);
    requireComponentVisibleWithin(strip, "authoringRedoButton", bounds);
    requireComponentVisibleWithin(strip, "authoringSaveButton", bounds);
    require(requireButton(strip, "authoringPreviewButton").isEnabled(),
            "Summary strip preview button should reflect the fixture view model.");
    require(requireButton(strip, "authoringUndoButton").isEnabled(),
            "Summary strip undo button should reflect the fixture view model.");
    require(requireButton(strip, "authoringRedoButton").isEnabled(),
            "Summary strip redo button should reflect the fixture view model.");

    require(static_cast<bool>(requireButton(strip, "authoringPreviewButton").onClick),
            "Summary strip preview button should expose an onClick callback.");
    requireButton(strip, "authoringPreviewButton").onClick();
    requireButton(strip, "authoringUndoButton").onClick();
    requireButton(strip, "authoringRedoButton").onClick();
    requireButton(strip, "authoringSaveButton").onClick();

    require(previewRequests == 1, "Summary strip should emit exactly one preview callback.");
    require(undoRequests == 1, "Summary strip should emit exactly one undo callback.");
    require(redoRequests == 1, "Summary strip should emit exactly one redo callback.");
    require(saveRequests == 1, "Summary strip should emit exactly one save callback.");

    saveComponentPng(strip, outputDirectory / "leaf-summary-strip.png");
}

void exerciseZoneMappingEditorLeaf(const fs::path& outputDirectory)
{
    drs::app::authoring::ZoneMappingEditor editor;
    editor.setTopLeftPosition(0, 0);
    editor.setSize(360, 520);
    editor.setVisible(true);

    drs::app::authoring::ZoneFieldValuesViewModel emptyViewModel;
    emptyViewModel.hasSelection = false;
    emptyViewModel.emptyStateText = "Select a zone to edit mapping fields.";
    editor.setViewModel(emptyViewModel);
    editor.resized();

    requireComponentVisibleWithin(editor, "authoringZoneFieldEditor", editor.getLocalBounds());
    requireComponentVisible(editor, "authoringZoneFieldEmptyState");
    require(!requireSlider(editor, "authoringRootKeySlider").isShowing(),
            "Zone mapping editor should hide mapping controls when no zone is selected.");

    int commitRequests = 0;
    int restoreRequests = 0;
    std::string lastCommitLabel;
    drs::app::authoring::ZoneFieldValuesViewModel lastCommittedValues;

    drs::app::authoring::ZoneFieldCallbacks callbacks;
    callbacks.onCommitRequested = [&](const drs::app::authoring::ZoneFieldValuesViewModel& values,
                                      const std::string& label)
    {
        ++commitRequests;
        lastCommitLabel = label;
        lastCommittedValues = values;
    };
    callbacks.onRestoreRootKeyRequested = [&restoreRequests]
    {
        ++restoreRequests;
    };
    editor.setCallbacks(std::move(callbacks));

    drs::app::authoring::ZoneFieldValuesViewModel populatedViewModel;
    populatedViewModel.hasSelection = true;
    populatedViewModel.rootKey = 60;
    populatedViewModel.keyLow = 48;
    populatedViewModel.keyHigh = 72;
    populatedViewModel.velocityLow = 8;
    populatedViewModel.velocityHigh = 120;
    populatedViewModel.gainDb = -3.5;
    populatedViewModel.pan = 0.25;
    populatedViewModel.loopEnabled = false;
    editor.setViewModel(populatedViewModel);
    editor.resized();

    const auto bounds = editor.getLocalBounds();
    require(countDescendantsById(editor, "authoringRootKeySlider") == 1,
            "Zone mapping editor should expose one root-key control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringKeyLowSlider") == 1,
            "Zone mapping editor should expose one key-low control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringKeyHighSlider") == 1,
            "Zone mapping editor should expose one key-high control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringVelocityLowSlider") == 1,
            "Zone mapping editor should expose one velocity-low control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringVelocityHighSlider") == 1,
            "Zone mapping editor should expose one velocity-high control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringGainSlider") == 1,
            "Zone mapping editor should expose one gain control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringPanSlider") == 1,
            "Zone mapping editor should expose one pan control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringLoopEnabledToggle") == 1,
            "Zone mapping editor should expose one loop toggle after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringRestoreRootKeyButton") == 1,
            "Zone mapping editor should expose one restore-root-key action after removing old mapping rows.");

    for (const auto& componentId : {
             juce::String("authoringMapInspectorSection"),
             juce::String("authoringSampleInspectorSection"),
             juce::String("authoringMixInspectorSection"),
             juce::String("authoringAdvancedInspectorSection"),
             juce::String("authoringMapInspectorSectionDisclosure"),
             juce::String("authoringSampleInspectorSectionDisclosure"),
             juce::String("authoringMixInspectorSectionDisclosure"),
             juce::String("authoringAdvancedInspectorSectionDisclosure")
         })
    {
        requireComponentVisibleWithin(editor, componentId, bounds);
    }

    for (const auto& componentId : {
             juce::String("authoringRootKeySlider"),
             juce::String("authoringKeyLowSlider"),
             juce::String("authoringKeyHighSlider")
         })
    {
        requireComponentVisibleWithin(editor, componentId, bounds);
    }

    if (findDescendantById(editor, "authoringVelocityLowSlider")->getBounds().isEmpty())
        requireButton(editor, "authoringSampleInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringVelocityLowSlider", bounds);
    requireComponentVisibleWithin(editor, "authoringVelocityHighSlider", bounds);
    requireButton(editor, "authoringSampleInspectorSectionDisclosure").onClick();

    if (findDescendantById(editor, "authoringGainSlider")->getBounds().isEmpty())
        requireButton(editor, "authoringMixInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringGainSlider", bounds);
    requireComponentVisibleWithin(editor, "authoringPanSlider", bounds);

    if (findDescendantById(editor, "authoringRestoreRootKeyButton")->getBounds().isEmpty())
        requireButton(editor, "authoringAdvancedInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringLoopEnabledToggle", bounds);
    requireComponentVisibleWithin(editor, "authoringRestoreRootKeyButton", bounds);
    requireComponentVisibleWithin(editor, "authoringZoneValidationMessage", bounds);

    auto& rootKeySlider = requireSlider(editor, "authoringRootKeySlider");
    require(static_cast<bool>(rootKeySlider.onDragStart),
            "Zone mapping editor root key slider should expose a drag-start commit callback.");
    rootKeySlider.setValue(67.0, juce::dontSendNotification);
    require(static_cast<bool>(rootKeySlider.onDragEnd),
            "Zone mapping editor root key slider should expose a drag-end commit callback.");
    rootKeySlider.onDragStart();
    rootKeySlider.onDragEnd();
    require(commitRequests == 1, "Zone mapping editor should commit root key edits.");
    require(lastCommitLabel == "Update zone root key", "Zone mapping editor should label root key edits.");
    require(lastCommittedValues.rootKey == 67, "Zone mapping editor should report the edited root key.");

    auto& panSlider = requireSlider(editor, "authoringPanSlider");
    panSlider.setValue(-0.4, juce::dontSendNotification);
    require(static_cast<bool>(panSlider.onDragEnd),
            "Zone mapping editor pan slider should expose a drag-end commit callback.");
    panSlider.onDragStart();
    panSlider.onDragEnd();
    require(commitRequests == 2, "Zone mapping editor should commit pan edits.");
    require(lastCommitLabel == "Update zone pan", "Zone mapping editor should label pan edits.");
    require(std::abs(lastCommittedValues.pan - (-0.4)) < 0.001,
            "Zone mapping editor should report the edited pan value.");

    auto& rootKeyTextBox = requireSliderTextBoxLabel(rootKeySlider);
    rootKeySlider.showTextBox();
    auto* rootKeyEditor = rootKeyTextBox.getCurrentTextEditor();
    require(rootKeyEditor != nullptr, "Zone mapping editor root key slider should show an editable text box.");
    rootKeyEditor->setText("64", false);
    require(commitRequests == 2,
            "Zone mapping editor should not commit root key text edits until the editor is dismissed.");
    rootKeyTextBox.hideEditor(false);
    require(commitRequests == 3, "Zone mapping editor should commit root key text edits when editing completes.");
    require(lastCommitLabel == "Update zone root key", "Zone mapping editor should preserve root key commit labels.");
    require(lastCommittedValues.rootKey == 64, "Zone mapping editor should report committed text-entry values.");

    auto& keyLowSlider = requireSlider(editor, "authoringKeyLowSlider");
    keyLowSlider.setValue(90.0, juce::dontSendNotification);
    keyLowSlider.onDragStart();
    keyLowSlider.onDragEnd();
    require(commitRequests == 4, "Zone mapping editor should commit key-range edits.");
    require(lastCommitLabel == "Update zone key range", "Zone mapping editor should label key-range edits.");
    require(lastCommittedValues.keyLow == 72 && lastCommittedValues.keyHigh == 90,
            "Zone mapping editor should normalize inverted key ranges before commit.");
    require(static_cast<int>(keyLowSlider.getValue()) == 72
                && static_cast<int>(requireSlider(editor, "authoringKeyHighSlider").getValue()) == 90,
            "Zone mapping editor should reflect normalized key ranges in the visible controls.");
    require(requireMessageText(editor, "authoringZoneValidationMessage")
                == "Key range was normalized to keep Low <= High.",
            "Zone mapping editor should explain when key ranges are normalized.");

    auto& velocityLowSlider = requireSlider(editor, "authoringVelocityLowSlider");
    velocityLowSlider.setValue(126.0, juce::dontSendNotification);
    velocityLowSlider.onDragStart();
    velocityLowSlider.onDragEnd();
    require(commitRequests == 5, "Zone mapping editor should commit velocity-range edits.");
    require(lastCommitLabel == "Update zone velocity range", "Zone mapping editor should label velocity-range edits.");
    require(lastCommittedValues.velocityLow == 120 && lastCommittedValues.velocityHigh == 126,
            "Zone mapping editor should normalize inverted velocity ranges before commit.");
    require(static_cast<int>(velocityLowSlider.getValue()) == 120
                && static_cast<int>(requireSlider(editor, "authoringVelocityHighSlider").getValue()) == 126,
            "Zone mapping editor should reflect normalized velocity ranges in the visible controls.");
    require(requireMessageText(editor, "authoringZoneValidationMessage")
                == "Velocity range was normalized to keep Low <= High.",
            "Zone mapping editor should explain when velocity ranges are normalized.");

    auto& loopToggle = requireToggleButton(editor, "authoringLoopEnabledToggle");
    loopToggle.setToggleState(true, juce::dontSendNotification);
    require(static_cast<bool>(loopToggle.onClick),
            "Zone mapping editor loop toggle should expose an onClick callback.");
    loopToggle.onClick();
    require(commitRequests == 6, "Zone mapping editor should commit loop toggle edits.");
    require(lastCommitLabel == "Toggle zone loop", "Zone mapping editor should label loop edits.");
    require(lastCommittedValues.loopEnabled, "Zone mapping editor should report the toggled loop state.");

    requireButton(editor, "authoringRestoreRootKeyButton").onClick();
    require(restoreRequests == 1, "Zone mapping editor should emit restore-root-key callbacks.");

    saveComponentPng(editor, outputDirectory / "leaf-zone-mapping-editor.png");
}

void writeReachabilityChecklist(std::ostream& inventory)
{
    inventory << "Reachability checklist\n";
    inventory << "- Summary strip: authoringSummaryStrip, authoringPreviewButton, authoringUndoButton, authoringRedoButton, authoringSaveButton\n";
    inventory << "- Toolbar row: authoringZoneSelector, authoringModeSelector\n";
    inventory << "- Persistent map: authoringZoneMap\n";
    inventory << "- Drawer host: authoringDrawer, authoringDrawerTabStrip, authoringDrawerToggleButton\n";
    inventory << "- Drawer tabs: authoringDrawerWaveformTab, authoringDrawerMacrosTab, authoringDrawerRoutingTab, authoringDrawerPerformanceTab\n";
    inventory << "- Mapping inspector: authoringRootKeySlider, authoringKeyLowSlider, authoringKeyHighSlider, authoringVelocityLowSlider, authoringVelocityHighSlider, authoringGainSlider, authoringPanSlider, authoringLoopEnabledToggle, authoringRestoreRootKeyButton\n";
    inventory << "- Waveform drawer content: authoringWaveformPreview plus waveform/loop/import labels\n";
    inventory << "- Macros inspector: authoringMacroSelector, authoringMacroAssignmentSelector, authoringMacroRoleSelector, authoringMacroDefaultSlider, authoringMacroMinSlider, authoringMacroMaxSlider, authoringMacroMoveUpButton, authoringMacroMoveDownButton\n";
    inventory << "- Routing inspector: authoringFxSelector, authoringFxTypeSelector, authoringFxBypassedToggle, authoringRoutingSelector, authoringRoutingInputSelector, authoringRoutingInsertOneSelector, authoringRoutingInsertTwoSelector\n";
    inventory << "- Performance inspector: authoringPerformanceBankSelector, authoringTriggerSlotSelector, authoringTriggerEventSelector, authoringTargetArticulationSelector, authoringPhraseAssetSelector, authoringChordModeSelector, authoringPhraseImportPath, authoringPhraseImportButton\n";
    inventory << "\n";
}

void exerciseMode(drs::app::AuthoringPanel& panel,
                  int modeId,
                  const std::string& shellName,
                  const std::string& modeName,
                  const fs::path& outputDirectory,
                  std::ostream& inventory,
                  std::vector<std::string>& baselineFindings)
{
    auto* modeSelector = dynamic_cast<juce::ComboBox*>(findDescendantById(panel, "authoringModeSelector"));
    require(modeSelector != nullptr, "Missing authoring mode selector.");
    modeSelector->setSelectedId(modeId, juce::sendNotificationSync);

    const auto panelBounds = panel.getLocalBounds();
    requireComponentVisibleWithin(panel, "authoringWorkspace", panelBounds);
    requireComponentVisibleWithin(panel, "authoringZoneSelector", panelBounds);
    requireComponentVisibleWithin(panel, "authoringZoneMap", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawer", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerTabStrip", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerToggleButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerWaveformTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerMacrosTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerRoutingTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerPerformanceTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPreviewButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringUndoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringRedoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringSaveButton", panelBounds);
    require(findDescendantById(panel, "authoringZoneMap")->getBounds().getHeight()
                >= drs::app::authoring::minimumMapVisibleHeight,
            "Authoring shell must preserve the minimum visible map height.");

    switch (modeId)
    {
        case 1:
            if (shellName == "expanded-min"
                && findDescendantById(panel, "authoringWaveformPreview")->isVisible())
            {
                requireButton(panel, "authoringDrawerToggleButton").onClick();
            }

            requireComponentVisibleWithin(panel, "authoringRootKeySlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringKeyLowSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringKeyHighSlider", panelBounds);
            require(countDescendantsById(panel, "authoringRootKeySlider") == 1,
                    "Authoring panel should not expose duplicate root-key controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringKeyLowSlider") == 1,
                    "Authoring panel should not expose duplicate key-low controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringKeyHighSlider") == 1,
                    "Authoring panel should not expose duplicate key-high controls after mapping-row removal.");

            if (findDescendantById(panel, "authoringVelocityLowSlider")->getBounds().isEmpty())
                requireButton(panel, "authoringSampleInspectorSectionDisclosure").onClick();
            requireComponentVisibleWithin(panel, "authoringVelocityLowSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringVelocityHighSlider", panelBounds);
            require(countDescendantsById(panel, "authoringVelocityLowSlider") == 1,
                    "Authoring panel should not expose duplicate velocity-low controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringVelocityHighSlider") == 1,
                    "Authoring panel should not expose duplicate velocity-high controls after mapping-row removal.");
            requireButton(panel, "authoringSampleInspectorSectionDisclosure").onClick();

            if (findDescendantById(panel, "authoringGainSlider")->getBounds().isEmpty())
                requireButton(panel, "authoringMixInspectorSectionDisclosure").onClick();
            requireComponentVisibleWithin(panel, "authoringGainSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringPanSlider", panelBounds);
            require(countDescendantsById(panel, "authoringGainSlider") == 1,
                    "Authoring panel should not expose duplicate gain controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringPanSlider") == 1,
                    "Authoring panel should not expose duplicate pan controls after mapping-row removal.");
            requireButton(panel, "authoringMixInspectorSectionDisclosure").onClick();

            if (findDescendantById(panel, "authoringRestoreRootKeyButton")->getBounds().isEmpty())
                requireButton(panel, "authoringAdvancedInspectorSectionDisclosure").onClick();
            requireComponentVisibleWithin(panel, "authoringLoopEnabledToggle", panelBounds);
            requireComponentVisible(panel, "authoringRestoreRootKeyButton");
            require(countDescendantsById(panel, "authoringLoopEnabledToggle") == 1,
                    "Authoring panel should not expose duplicate loop toggles after mapping-row removal.");
            require(countDescendantsById(panel, "authoringRestoreRootKeyButton") == 1,
                    "Authoring panel should not expose duplicate restore-root-key actions after mapping-row removal.");
            break;
        case 2:
            requireComponentVisibleWithin(panel, "authoringMacroSelector", panelBounds);
            break;
        case 3:
            requireComponentVisibleWithin(panel, "authoringFxSelector", panelBounds);
            requireComponentVisibleWithin(panel, "authoringRoutingSelector", panelBounds);
            break;
        case 4:
            requireComponentVisibleWithin(panel, "authoringPerformanceBankSelector", panelBounds);
            requireComponentVisibleWithin(panel, "authoringTriggerSlotSelector", panelBounds);
            break;
        default:
            require(false, "Unexpected authoring mode ID.");
            break;
    }

    inventory << shellName << " / " << modeName << "\n";
    for (const auto& componentId : {
             juce::String("authoringWorkspace"),
             juce::String("authoringZoneSelector"),
             juce::String("authoringDrawer"),
             juce::String("authoringDrawerTabStrip"),
             juce::String("authoringWaveformPreview"),
             juce::String("authoringZoneMap"),
             juce::String("authoringGainSlider"),
             juce::String("authoringPanSlider"),
             juce::String("authoringRestoreRootKeyButton"),
             juce::String("authoringMacroSelector"),
             juce::String("authoringFxSelector"),
             juce::String("authoringRoutingSelector"),
             juce::String("authoringPerformanceBankSelector"),
             juce::String("authoringTriggerSlotSelector")
         })
    {
        inventory << "  " << describeBounds(panel, componentId) << "\n";
    }

    std::vector<juce::String> activeModeComponents;
    switch (modeId)
    {
        case 1:
            activeModeComponents = {
                "authoringZoneMap",
                "authoringRootKeySlider",
                "authoringKeyLowSlider",
                "authoringKeyHighSlider"
            };
            break;
        case 2:
            activeModeComponents = {
                "authoringMacroSelector",
                "authoringMacroAssignmentSelector",
                "authoringMacroRoleSelector",
                "authoringMacroDefaultSlider",
                "authoringMacroMinSlider",
                "authoringMacroMaxSlider",
                "authoringMacroMoveUpButton",
                "authoringMacroMoveDownButton"
            };
            break;
        case 3:
            activeModeComponents = {
                "authoringFxSelector",
                "authoringFxTypeSelector",
                "authoringFxBypassedToggle",
                "authoringRoutingSelector",
                "authoringRoutingInputSelector",
                "authoringRoutingInsertOneSelector",
                "authoringRoutingInsertTwoSelector"
            };
            break;
        case 4:
            activeModeComponents = {
                "authoringPerformanceBankSelector",
                "authoringTriggerSlotSelector",
                "authoringTriggerEventSelector",
                "authoringTargetArticulationSelector",
                "authoringPhraseAssetSelector",
                "authoringChordModeSelector",
                "authoringPhraseImportPath",
                "authoringPhraseImportButton"
            };
            break;
        default:
            break;
    }

    for (const auto& componentId : activeModeComponents)
    {
        if (auto* component = findDescendantById(panel, componentId);
            component == nullptr)
        {
            baselineFindings.push_back(shellName + " / " + modeName + " missing: " + componentId.toStdString());
        }
        else if (!component->isVisible())
        {
            baselineFindings.push_back(shellName + " / " + modeName + " hidden: " + componentId.toStdString());
        }
        else if (component->getBounds().isEmpty())
        {
            baselineFindings.push_back(shellName + " / " + modeName + " empty-bounds: " + componentId.toStdString());
        }
        else if (!panelBounds.contains(component->getBounds()))
        {
            baselineFindings.push_back(shellName + " / " + modeName + " clipped: " + componentId.toStdString());
        }
    }

    inventory << "\n";

    saveComponentPng(panel, outputDirectory / (shellName + "-" + modeName + ".png"));
}

void exerciseDrawerBehavior(drs::app::AuthoringPanel& panel,
                            const std::string& shellName,
                            std::vector<std::string>& baselineFindings)
{
    const auto panelBounds = panel.getLocalBounds();
    auto& toggleButton = requireButton(panel, "authoringDrawerToggleButton");
    auto& waveformTabButton = requireButton(panel, "authoringDrawerWaveformTab");
    auto& macrosTabButton = requireButton(panel, "authoringDrawerMacrosTab");
    auto* zoneSelector = dynamic_cast<juce::ComboBox*>(findDescendantById(panel, "authoringZoneSelector"));

    require(zoneSelector != nullptr, "Drawer behavior checks require the zone selector.");

    if (shellName == "compact")
    {
        require(!requireSlider(panel, "authoringRootKeySlider").getBounds().isEmpty(),
                "Compact shell should still lay out mapping controls before drawer checks.");
        require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
                "Compact shell should begin with the drawer content hidden.");
        toggleButton.onClick();
        requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);
    }
    else
    {
        requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);
    }

    macrosTabButton.onClick();
    requireComponentVisibleWithin(panel, "authoringDrawerPlaceholder", panelBounds);
    require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
            "Non-waveform drawer tabs should hide waveform content during Sprint 2.");

    zoneSelector->setSelectedId(2, juce::sendNotificationSync);
    if (auto* placeholder = findDescendantById(panel, "authoringDrawerPlaceholder");
        placeholder == nullptr || !placeholder->isVisible())
    {
        baselineFindings.push_back(shellName + " / drawer state did not survive zone selection");
    }

    waveformTabButton.onClick();
    requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);

    if (shellName == "compact")
    {
        toggleButton.onClick();
        require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
                "Compact shell drawer should close back to its default collapsed state.");
    }
}

void exerciseMapSelectionBehavior(drs::app::AuthoringPanel& panel,
                                  drs::engine::AuthoringSession& session)
{
    auto* modeSelector = dynamic_cast<juce::ComboBox*>(findDescendantById(panel, "authoringModeSelector"));
    require(modeSelector != nullptr, "Map selection checks require the authoring mode selector.");
    modeSelector->setSelectedId(1, juce::sendNotificationSync);

    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& rootKeySlider = requireSlider(panel, "authoringRootKeySlider");

    const auto initialUndoDepth = session.getDocumentState().undoDepth;
    const auto summaries = session.getZoneSummaries();
    require(summaries.size() >= 3, "Map selection checks require the Phase 2 reference zones.");

    const auto padIterator = std::find_if(summaries.begin(),
                                          summaries.end(),
                                          [](const auto& zone)
                                          {
                                              return zone.id == "pad-a3-high";
                                          });
    require(padIterator != summaries.end(), "Map selection checks require the pad-a3-high zone.");

    const auto padIndex = static_cast<int>(std::distance(summaries.begin(), padIterator));
    require(zoneMap.requestSelectionAt(computeZoneMapPoint(zoneMap, *padIterator)),
            "Zone map should select a zone when clicked within its bounds.");
    require(session.getSelectedZone()->id == "pad-a3-high",
            "Zone map click selection should retarget the selected session zone.");
    require(zoneSelector.getSelectedId() == padIndex + 1,
            "Zone selector should stay synchronized with map click selection.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should refresh to the map-selected zone values.");
    require(componentTreeContainsLabelText(panel,
                                           "Sample source: "
                                               + juce::String::fromUTF8(session.getSelectedZone()->sampleSourceId.c_str())),
            "Summary strip should refresh its sample-source text after map click selection.");

    require(zoneMap.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)),
            "Zone map should accept right-arrow keyboard navigation.");
    require(session.getSelectedZone()->id == "lead-a4-sustain",
            "Zone map keyboard navigation should advance to the next zone.");
    require(zoneSelector.getSelectedId() == static_cast<int>(summaries.size()),
            "Zone selector should stay synchronized with map keyboard navigation.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should refresh after keyboard-driven map selection changes.");
    require(componentTreeContainsLabelText(panel,
                                           "Sample source: "
                                               + juce::String::fromUTF8(session.getSelectedZone()->sampleSourceId.c_str())),
            "Summary strip should refresh after keyboard-driven map selection changes.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 2,
            "Map click and keyboard selection should each create one selection transaction.");

    const auto selectedZoneBeforeDrag = makeZoneSummary(*session.getSelectedZone());
    const auto keyLowHandle = computeZoneMapHandlePoint(zoneMap,
                                                        selectedZoneBeforeDrag,
                                                        drs::app::authoring::ZoneMapCanvas::RangeHandle::keyLow);
    require(zoneMap.beginRangeGestureAt(keyLowHandle),
            "Zone map should begin a key-range gesture from the selected zone handle.");
    require(zoneMap.isRangeGestureActive(),
            "Zone map should report an active gesture while a range drag is in progress.");
    const auto keyDragTarget = computeZoneMapTargetPointForKey(zoneMap, selectedZoneBeforeDrag, 72);
    require(zoneMap.updateActiveRangeGesture(keyDragTarget),
            "Zone map should update the live key-range gesture preview.");
    require(zoneMap.endActiveRangeGesture(keyDragTarget),
            "Zone map should finish a key-range gesture on release.");
    require(!zoneMap.isRangeGestureActive(),
            "Zone map should clear its active gesture state after commit.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 3,
            "Completing one map range drag should create exactly one additional undo transaction.");
    require(session.getSelectedZone()->keyLow == 72,
            "Zone map key-range drags should persist the committed low-key boundary.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should remain synchronized after map range edits.");
    require(static_cast<int>(requireSlider(panel, "authoringKeyLowSlider").getValue()) == 72,
            "Zone inspector key-low control should refresh after a committed map drag.");

    const auto zoneBeforeCancel = makeZoneSummary(*session.getSelectedZone());
    const auto velocityLowHandle = computeZoneMapHandlePoint(zoneMap,
                                                             zoneBeforeCancel,
                                                             drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityLow);
    require(zoneMap.beginRangeGestureAt(velocityLowHandle),
            "Zone map should begin a velocity-range gesture from the selected zone handle.");
    const auto velocityDragTarget = computeZoneMapTargetPointForVelocity(zoneMap, zoneBeforeCancel, 32);
    require(zoneMap.updateActiveRangeGesture(velocityDragTarget),
            "Zone map should update the live velocity-range gesture preview.");
    require(zoneMap.cancelActiveRangeGesture(),
            "Zone map should cancel an in-flight drag without committing.");
    require(!zoneMap.isRangeGestureActive(),
            "Zone map should clear its active gesture state after cancel.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 3,
            "Cancelling a map range drag should not create a new undo transaction.");
    require(session.getSelectedZone()->velocityLow == zoneBeforeCancel.velocityLow,
            "Cancelling a map range drag should preserve the original velocity range.");
    require(static_cast<int>(requireSlider(panel, "authoringVelocityLowSlider").getValue())
                == zoneBeforeCancel.velocityLow,
            "Zone inspector should remain unchanged after a cancelled map drag.");
}

void exerciseGateAWorkflow(drs::app::AuthoringPanel& panel,
                           drs::engine::AuthoringSession& session,
                           const std::string& shellName,
                           const fs::path& outputDirectory,
                           std::ostream& inventory,
                           int& previewStartCount,
                           int& previewEndCount,
                           int& restoreRootKeyCount,
                           int& lastPreviewMidiNote,
                           float& lastPreviewVelocity)
{
    auto* modeSelector = dynamic_cast<juce::ComboBox*>(findDescendantById(panel, "authoringModeSelector"));
    require(modeSelector != nullptr, "Gate A workflow requires the authoring mode selector.");
    modeSelector->setSelectedId(1, juce::sendNotificationSync);

    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& previewButton = requireButton(panel, "authoringPreviewButton");
    auto& undoButton = requireButton(panel, "authoringUndoButton");
    auto& redoButton = requireButton(panel, "authoringRedoButton");
    auto& saveButton = requireButton(panel, "authoringSaveButton");

    zoneSelector.setSelectedId(2, juce::sendNotificationSync);
    require(session.getSelectedZone()->id == "pad-a3-high",
            "Gate A workflow zone selection should retarget the selected zone.");

    const auto originalRootKey = session.getSelectedZone()->rootKey;
    const auto undoDepthAfterSelection = session.getDocumentState().undoDepth;

    auto ensureSectionOpen = [&](const juce::String& targetComponentId, const juce::String& disclosureButtonId)
    {
        if (findDescendantById(panel, targetComponentId)->getBounds().isEmpty())
            requireButton(panel, disclosureButtonId).onClick();
    };

    const auto selectedZone = makeZoneSummary(*session.getSelectedZone());
    const auto keyHighHandle = computeZoneMapHandlePoint(zoneMap,
                                                         selectedZone,
                                                         drs::app::authoring::ZoneMapCanvas::RangeHandle::keyHigh);
    const auto keyHighTarget = computeZoneMapTargetPointForKey(zoneMap, selectedZone, 84);
    require(zoneMap.beginRangeGestureAt(keyHighHandle),
            "Gate A workflow should begin the map key-range gesture.");
    require(zoneMap.updateActiveRangeGesture(keyHighTarget),
            "Gate A workflow should update the map key-range gesture.");
    require(zoneMap.endActiveRangeGesture(keyHighTarget),
            "Gate A workflow should commit the map key-range gesture.");
    require(session.getSelectedZone()->keyHigh == 84,
            "Gate A workflow key-range drag should persist the edited key-high boundary.");

    const auto zoneAfterKeyDrag = makeZoneSummary(*session.getSelectedZone());
    const auto velocityHighHandle = computeZoneMapHandlePoint(zoneMap,
                                                              zoneAfterKeyDrag,
                                                              drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityHigh);
    const auto velocityHighTarget = computeZoneMapTargetPointForVelocity(zoneMap, zoneAfterKeyDrag, 112);
    require(zoneMap.beginRangeGestureAt(velocityHighHandle),
            "Gate A workflow should begin the map velocity-range gesture.");
    require(zoneMap.updateActiveRangeGesture(velocityHighTarget),
            "Gate A workflow should update the map velocity-range gesture.");
    require(zoneMap.endActiveRangeGesture(velocityHighTarget),
            "Gate A workflow should commit the map velocity-range gesture.");
    require(session.getSelectedZone()->velocityHigh == 112,
            "Gate A workflow velocity-range drag should persist the edited velocity-high boundary.");

    auto& rootKeySlider = requireSlider(panel, "authoringRootKeySlider");
    rootKeySlider.onDragStart();
    rootKeySlider.setValue(64.0, juce::dontSendNotification);
    rootKeySlider.onDragEnd();
    require(session.getSelectedZone()->rootKey == 64,
            "Gate A workflow should commit root-key edits through the compact inspector.");

    ensureSectionOpen("authoringGainSlider", "authoringMixInspectorSectionDisclosure");
    auto& gainSlider = requireSlider(panel, "authoringGainSlider");
    gainSlider.onDragStart();
    gainSlider.setValue(1.5, juce::dontSendNotification);
    gainSlider.onDragEnd();
    require(std::abs(session.getSelectedZone()->gainDb - 1.5) < 0.001,
            "Gate A workflow should commit gain edits through the compact inspector.");

    auto& panSlider = requireSlider(panel, "authoringPanSlider");
    panSlider.onDragStart();
    panSlider.setValue(-0.2, juce::dontSendNotification);
    panSlider.onDragEnd();
    require(std::abs(session.getSelectedZone()->pan - (-0.2)) < 0.001,
            "Gate A workflow should commit pan edits through the compact inspector.");

    ensureSectionOpen("authoringRestoreRootKeyButton", "authoringAdvancedInspectorSectionDisclosure");
    auto& loopToggle = requireToggleButton(panel, "authoringLoopEnabledToggle");
    loopToggle.setToggleState(true, juce::dontSendNotification);
    loopToggle.onClick();
    require(session.getSelectedZone()->loopEnabled,
            "Gate A workflow should commit loop-toggle edits through the compact inspector.");

    const auto expectedPreview = session.buildSelectedZonePreviewRequest();
    require(expectedPreview.available, "Gate A workflow preview should be available after zone edits.");
    previewButton.onClick();
    require(previewStartCount == 1, "Gate A workflow should emit one preview-start callback.");
    require(lastPreviewMidiNote == expectedPreview.midiNote,
            "Gate A workflow preview callback should use the selected-zone preview note.");
    require(std::abs(lastPreviewVelocity - (static_cast<float>(expectedPreview.velocity) / 127.0f)) < 0.001f,
            "Gate A workflow preview callback should use the selected-zone preview velocity.");

    const auto undoDepthBeforeRestore = session.getDocumentState().undoDepth;
    requireButton(panel, "authoringRestoreRootKeyButton").onClick();
    require(restoreRootKeyCount == 1, "Gate A workflow should invoke restore-root-key exactly once.");
    require(session.getSelectedZone()->rootKey == originalRootKey,
            "Gate A workflow restore-root-key action should restore the zone root key.");
    require(session.getDocumentState().undoDepth == undoDepthBeforeRestore + 1,
            "Gate A workflow restore-root-key action should create one undo transaction.");

    undoButton.onClick();
    require(session.getSelectedZone()->rootKey == 64,
            "Gate A workflow undo should restore the pre-restore root key.");
    redoButton.onClick();
    require(session.getSelectedZone()->rootKey == originalRootKey,
            "Gate A workflow redo should reapply the restored root key.");

    require(session.getDocumentState().dirty,
            "Gate A workflow should leave the project dirty before marking a save checkpoint.");
    saveButton.onClick();
    require(!session.getDocumentState().dirty,
            "Gate A workflow mark-saved action should clear the dirty flag.");
    require(session.getDocumentState().undoDepth >= undoDepthAfterSelection + 6,
            "Gate A workflow should leave behind the expected edit history depth.");

    inventory << shellName << " / gate-a\n";
    inventory << "  zone=" << session.getSelectedZone()->id
              << " key=" << session.getSelectedZone()->keyLow << "-" << session.getSelectedZone()->keyHigh
              << " vel=" << session.getSelectedZone()->velocityLow << "-" << session.getSelectedZone()->velocityHigh
              << " root=" << session.getSelectedZone()->rootKey
              << " gain=" << session.getSelectedZone()->gainDb
              << " pan=" << session.getSelectedZone()->pan
              << " loop=" << (session.getSelectedZone()->loopEnabled ? "true" : "false")
              << " dirty=" << (session.getDocumentState().dirty ? "true" : "false")
              << " undo=" << session.getDocumentState().undoDepth
              << " redo=" << session.getDocumentState().redoDepth
              << " previewMidi=" << lastPreviewMidiNote
              << " previewVelocity=" << lastPreviewVelocity
              << " previewEndCallbacks=" << previewEndCount
              << "\n\n";

    saveComponentPng(panel, outputDirectory / (shellName + "-gate-a-final.png"));
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference project must load for authoring UI characterization.");

        const auto outputDirectory = fs::temp_directory_path() / "drs-phase2-authoring-ui-tests";
        fs::create_directories(outputDirectory);
        std::ofstream inventory(outputDirectory / "component-bounds.txt", std::ios::binary);
        require(inventory.good(), "Could not create authoring UI component inventory output.");
        std::vector<std::string> baselineFindings;

        exerciseSummaryStripLeaf(outputDirectory);
        exerciseZoneMappingEditorLeaf(outputDirectory);
        writeReachabilityChecklist(inventory);

        auto runShell = [&](const std::string& shellName,
                            drs::app::AuthoringPanel::LayoutMode layoutMode,
                            int width,
                            int height)
        {
            drs::engine::AuthoringSession session(projectLoad.project);
            int previewStartCount = 0;
            int previewEndCount = 0;
            int restoreRootKeyCount = 0;
            int lastPreviewMidiNote = -1;
            float lastPreviewVelocity = -1.0f;
            drs::app::AuthoringPanel* panelPtr = nullptr;
            drs::app::AuthoringPanel panel(session,
                                           []()
                                           {
                                               return makePreviewFixture();
                                           },
                                           []()
                                           {
                                               return makeImportMetricsFixture();
                                           },
                                           layoutMode,
                                           [&previewStartCount, &lastPreviewMidiNote, &lastPreviewVelocity](int midiNote, float velocity)
                                           {
                                               ++previewStartCount;
                                               lastPreviewMidiNote = midiNote;
                                               lastPreviewVelocity = velocity;
                                           },
                                           [&previewEndCount](int)
                                           {
                                               ++previewEndCount;
                                           },
                                           [&session, &restoreRootKeyCount, &panelPtr]()
                                           {
                                               ++restoreRootKeyCount;

                                               const auto selectedZone = session.getSelectedZone();
                                               if (!selectedZone.has_value())
                                                   return;

                                               auto restoredZone = *selectedZone;
                                               restoredZone.rootKey = restoredZone.id == "pad-a3-high" ? 57 : 69;
                                               session.updateSelectedZone(restoredZone, "Restore zone root key");

                                               if (panelPtr != nullptr)
                                                   panelPtr->reloadFromSession();
                                           });
            panelPtr = &panel;
            panel.setTopLeftPosition(0, 0);
            panel.setSize(width, height);
            panel.setVisible(true);
            panel.resized();
            panel.reloadFromSession();

            require(panel.getWidth() == width && panel.getHeight() == height,
                    "Authoring panel size did not match the requested shell baseline.");

            exerciseMapSelectionBehavior(panel, session);
            exerciseGateAWorkflow(panel,
                                  session,
                                  shellName,
                                  outputDirectory,
                                  inventory,
                                  previewStartCount,
                                  previewEndCount,
                                  restoreRootKeyCount,
                                  lastPreviewMidiNote,
                                  lastPreviewVelocity);
            exerciseDrawerBehavior(panel, shellName, baselineFindings);
            exerciseMode(panel, 1, shellName, "mapping", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 2, shellName, "macros", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 3, shellName, "routing", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 4, shellName, "performance", outputDirectory, inventory, baselineFindings);
        };

        runShell("compact",
                 drs::app::AuthoringPanel::LayoutMode::compact,
                 drs::app::authoring::compactShellWidth,
                 drs::app::authoring::compactShellHeight);

        runShell("expanded-target",
                 drs::app::AuthoringPanel::LayoutMode::expanded,
                 drs::app::authoring::expandedTargetShellWidth,
                 drs::app::authoring::expandedTargetShellHeight);

        runShell("expanded-min",
                 drs::app::AuthoringPanel::LayoutMode::expanded,
                 drs::app::authoring::expandedMinimumShellWidth,
                 drs::app::authoring::minimumShellHeight);

        inventory << "Baseline findings\n";
        for (const auto& finding : baselineFindings)
            inventory << "- " << finding << "\n";
        require(baselineFindings.empty(),
                "Authoring UI baseline findings must be empty after Sprint 1 layout hardening.");

        std::cout << "Phase 2 authoring UI characterization tests passed. Output: "
                  << outputDirectory.string() << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring UI characterization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
