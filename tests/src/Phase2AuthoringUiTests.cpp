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

juce::ToggleButton& requireToggleButton(juce::Component& root, const juce::String& componentId)
{
    auto* toggle = dynamic_cast<juce::ToggleButton*>(findDescendantById(root, componentId));
    require(toggle != nullptr, "Missing toggle ID: " + componentId.toStdString());
    return *toggle;
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
    drs::app::authoring::ZoneMappingEditor editor(drs::app::authoring::ZoneMappingEditor::LayoutMode::compact);
    editor.setTopLeftPosition(0, 0);
    editor.setSize(360, 180);
    editor.setVisible(true);

    drs::app::authoring::ZoneFieldValuesViewModel emptyViewModel;
    emptyViewModel.hasSelection = false;
    emptyViewModel.emptyStateText = "Select a zone to edit mapping fields.";
    editor.setViewModel(emptyViewModel);
    editor.resized();

    requireComponentVisibleWithin(editor, "authoringZoneFieldEditor", editor.getLocalBounds());
    requireComponentVisible(editor, "authoringZoneFieldEmptyState");
    require(!requireSlider(editor, "authoringRootKeySlider").isVisible(),
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
    for (const auto& componentId : {
             juce::String("authoringRootKeySlider"),
             juce::String("authoringKeyLowSlider"),
             juce::String("authoringKeyHighSlider"),
             juce::String("authoringVelocityLowSlider"),
             juce::String("authoringVelocityHighSlider"),
             juce::String("authoringGainSlider"),
             juce::String("authoringPanSlider"),
             juce::String("authoringLoopEnabledToggle"),
             juce::String("authoringRestoreRootKeyButton")
         })
    {
        requireComponentVisibleWithin(editor, componentId, bounds);
    }

    auto& rootKeySlider = requireSlider(editor, "authoringRootKeySlider");
    rootKeySlider.setValue(67.0, juce::dontSendNotification);
    require(static_cast<bool>(rootKeySlider.onDragEnd),
            "Zone mapping editor root key slider should expose a drag-end commit callback.");
    rootKeySlider.onDragEnd();
    require(commitRequests == 1, "Zone mapping editor should commit root key edits.");
    require(lastCommitLabel == "Update zone root key", "Zone mapping editor should label root key edits.");
    require(lastCommittedValues.rootKey == 67, "Zone mapping editor should report the edited root key.");

    auto& panSlider = requireSlider(editor, "authoringPanSlider");
    panSlider.setValue(-0.4, juce::dontSendNotification);
    require(static_cast<bool>(panSlider.onDragEnd),
            "Zone mapping editor pan slider should expose a drag-end commit callback.");
    panSlider.onDragEnd();
    require(commitRequests == 2, "Zone mapping editor should commit pan edits.");
    require(lastCommitLabel == "Update zone pan", "Zone mapping editor should label pan edits.");
    require(std::abs(lastCommittedValues.pan - (-0.4)) < 0.001,
            "Zone mapping editor should report the edited pan value.");

    auto& loopToggle = requireToggleButton(editor, "authoringLoopEnabledToggle");
    loopToggle.setToggleState(true, juce::dontSendNotification);
    require(static_cast<bool>(loopToggle.onClick),
            "Zone mapping editor loop toggle should expose an onClick callback.");
    loopToggle.onClick();
    require(commitRequests == 3, "Zone mapping editor should commit loop toggle edits.");
    require(lastCommitLabel == "Toggle zone loop", "Zone mapping editor should label loop edits.");
    require(lastCommittedValues.loopEnabled, "Zone mapping editor should report the toggled loop state.");

    requireButton(editor, "authoringRestoreRootKeyButton").onClick();
    require(restoreRequests == 1, "Zone mapping editor should emit restore-root-key callbacks.");

    saveComponentPng(editor, outputDirectory / "leaf-zone-mapping-editor.png");
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
    requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPreviewButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringUndoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringRedoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringSaveButton", panelBounds);

    switch (modeId)
    {
        case 1:
            requireComponentVisibleWithin(panel, "authoringZoneMap", panelBounds);
            requireComponentVisible(panel, "authoringRestoreRootKeyButton");
            require(findDescendantById(panel, "authoringZoneMap")->getBounds().getHeight()
                        >= drs::app::authoring::minimumMapVisibleHeight,
                    "Mapping mode must preserve the minimum visible map height.");
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
             juce::String("authoringWaveformPreview"),
             juce::String("authoringZoneMap"),
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
                "authoringKeyHighSlider",
                "authoringVelocityLowSlider",
                "authoringVelocityHighSlider",
                "authoringGainSlider",
                "authoringPanSlider",
                "authoringLoopEnabledToggle",
                "authoringRestoreRootKeyButton"
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

        auto runShell = [&](const std::string& shellName,
                            drs::app::AuthoringPanel::LayoutMode layoutMode,
                            int width,
                            int height)
        {
            drs::engine::AuthoringSession session(projectLoad.project);
            drs::app::AuthoringPanel panel(session,
                                           []()
                                           {
                                               return makePreviewFixture();
                                           },
                                           []()
                                           {
                                               return makeImportMetricsFixture();
                                           },
                                           layoutMode);
            panel.setTopLeftPosition(0, 0);
            panel.setSize(width, height);
            panel.setVisible(true);
            panel.resized();
            panel.reloadFromSession();

            require(panel.getWidth() == width && panel.getHeight() == height,
                    "Authoring panel size did not match the requested shell baseline.");

            exerciseMode(panel, 1, shellName, "mapping", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 2, shellName, "macros", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 3, shellName, "routing", outputDirectory, inventory, baselineFindings);
            exerciseMode(panel, 4, shellName, "performance", outputDirectory, inventory, baselineFindings);
        };

        runShell("compact",
                 drs::app::AuthoringPanel::LayoutMode::compact,
                 drs::app::authoring::compactShellWidth,
                 drs::app::authoring::compactShellHeight);

        runShell("expanded",
                 drs::app::AuthoringPanel::LayoutMode::expanded,
                 drs::app::authoring::expandedBaselineShellWidth,
                 drs::app::authoring::expandedBaselineShellHeight);

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
