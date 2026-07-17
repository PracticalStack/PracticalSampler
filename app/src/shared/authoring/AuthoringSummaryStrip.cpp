#include "shared/authoring/AuthoringSummaryStrip.h"

namespace drs::app::authoring
{
namespace
{
const auto summaryMuted = juce::Colour::fromRGB(82, 86, 94);

void configureAccessibleMetadata(juce::Component& component,
                                 const juce::String& title,
                                 const juce::String& description,
                                 const juce::String& helpText = {})
{
    component.setTitle(title);
    component.setDescription(description);

    if (helpText.isNotEmpty())
        component.setHelpText(helpText);
}

void updateDynamicAccessibleText(juce::Component& component,
                                 const juce::String& text,
                                 const juce::String& descriptionPrefix)
{
    component.setTitle(text);
    component.setDescription(descriptionPrefix + text);
}

void updateActionAccessibilityState(juce::Component& component,
                                    bool enabled,
                                    const juce::String& enabledDescription,
                                    const juce::String& disabledDescription,
                                    const juce::String& enabledHelpText,
                                    const juce::String& disabledHelpText)
{
    component.setDescription(enabled ? enabledDescription : disabledDescription);
    component.setHelpText(enabled ? enabledHelpText : disabledHelpText);
}
} // namespace

AuthoringSummaryStrip::AuthoringSummaryStrip()
{
    setComponentID("authoringSummaryStrip");
    configureAccessibleMetadata(*this,
                                "Selection summary strip",
                                "Summarizes the current zone selection and exposes preview, undo, redo, and save actions.");

    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setComponentID("authoringSummaryTitleLabel");
    configureAccessibleMetadata(titleLabel,
                                "Selected zone title",
                                "Displays the selected zone name.");

    statusLabel.setColour(juce::Label::textColourId, summaryMuted);
    sourceLabel.setColour(juce::Label::textColourId, summaryMuted);
    articulationLabel.setColour(juce::Label::textColourId, summaryMuted);
    statusLabel.setComponentID("authoringSummaryStatusLabel");
    sourceLabel.setComponentID("authoringSummarySourceLabel");
    articulationLabel.setComponentID("authoringSummaryArticulationLabel");
    configureAccessibleMetadata(statusLabel,
                                "Selection status",
                                "Displays the current selection state.");
    configureAccessibleMetadata(sourceLabel,
                                "Selected zone source",
                                "Displays the selected zone sample source.");
    configureAccessibleMetadata(articulationLabel,
                                "Selected zone articulation",
                                "Displays the selected zone articulation.");

    previewButton.setButtonText("Preview Selected Zone");
    previewButton.setComponentID("authoringPreviewButton");
    configureAccessibleMetadata(previewButton,
                                "Preview selected zone",
                                "Starts a preview for the selected zone.",
                                "Press to audition the selected zone.");
    previewButton.setExplicitFocusOrder(13);
    previewButton.onClick = [this]
    {
        if (callbacks.onPreviewRequested)
            callbacks.onPreviewRequested();
    };

    undoButton.setButtonText("Undo");
    undoButton.setComponentID("authoringUndoButton");
    configureAccessibleMetadata(undoButton,
                                "Undo",
                                "Reverts the most recent authoring change.",
                                "Press to undo the last change.");
    undoButton.setExplicitFocusOrder(10);
    undoButton.onClick = [this]
    {
        if (callbacks.onUndoRequested)
            callbacks.onUndoRequested();
    };

    redoButton.setButtonText("Redo");
    redoButton.setComponentID("authoringRedoButton");
    configureAccessibleMetadata(redoButton,
                                "Redo",
                                "Reapplies the most recently undone authoring change.",
                                "Press to redo the last undone change.");
    redoButton.setExplicitFocusOrder(11);
    redoButton.onClick = [this]
    {
        if (callbacks.onRedoRequested)
            callbacks.onRedoRequested();
    };

    saveCheckpointButton.setButtonText("Mark Saved");
    saveCheckpointButton.setComponentID("authoringSaveButton");
    configureAccessibleMetadata(saveCheckpointButton,
                                "Mark saved",
                                "Marks the current authoring state as saved.",
                                "Press to clear the dirty state after reviewing changes.");
    saveCheckpointButton.setExplicitFocusOrder(12);
    saveCheckpointButton.onClick = [this]
    {
        if (callbacks.onMarkSavedRequested)
            callbacks.onMarkSavedRequested();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&titleLabel),
             static_cast<juce::Component*>(&statusLabel),
             static_cast<juce::Component*>(&sourceLabel),
             static_cast<juce::Component*>(&articulationLabel),
             static_cast<juce::Component*>(&previewButton),
             static_cast<juce::Component*>(&undoButton),
             static_cast<juce::Component*>(&redoButton),
             static_cast<juce::Component*>(&saveCheckpointButton)
         })
    {
        addAndMakeVisible(component);
    }
}

void AuthoringSummaryStrip::resized()
{
    auto hero = getLocalBounds();
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
}

void AuthoringSummaryStrip::setViewModel(SelectionSummaryViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    titleLabel.setText(juce::String::fromUTF8(viewModel.title.c_str()), juce::dontSendNotification);
    statusLabel.setText(juce::String::fromUTF8(viewModel.statusText.c_str()), juce::dontSendNotification);
    sourceLabel.setText(juce::String::fromUTF8(viewModel.sourceText.c_str()), juce::dontSendNotification);
    articulationLabel.setText(juce::String::fromUTF8(viewModel.articulationText.c_str()), juce::dontSendNotification);
    updateDynamicAccessibleText(titleLabel, titleLabel.getText(), "Selected zone title: ");
    updateDynamicAccessibleText(statusLabel, statusLabel.getText(), "Selection status: ");
    updateDynamicAccessibleText(sourceLabel, sourceLabel.getText(), "Selected zone source: ");
    updateDynamicAccessibleText(articulationLabel, articulationLabel.getText(), "Selected zone articulation: ");

    previewButton.setEnabled(viewModel.canPreview);
    undoButton.setEnabled(viewModel.canUndo);
    redoButton.setEnabled(viewModel.canRedo);
    updateActionAccessibilityState(previewButton,
                                   viewModel.canPreview,
                                   "Previews the selected zone.",
                                   "Unavailable because no zone preview is available.",
                                   "Press to audition the selected zone.",
                                   "Select a zone to enable preview.");
    updateActionAccessibilityState(undoButton,
                                   viewModel.canUndo,
                                   "Reverts the most recent authoring change.",
                                   "Unavailable because there is no change to undo.",
                                   "Press to undo the last change.",
                                   "Make a change to enable undo.");
    updateActionAccessibilityState(redoButton,
                                   viewModel.canRedo,
                                   "Reapplies the most recently undone authoring change.",
                                   "Unavailable because there is no change to redo.",
                                   "Press to redo the last undone change.",
                                   "Undo a change to enable redo.");
    saveCheckpointButton.setDescription(viewModel.dirty
                                            ? "Marks the current authoring state as saved."
                                            : "Project is already marked saved.");
    saveCheckpointButton.setHelpText(viewModel.dirty
                                         ? "Press to clear the dirty state after reviewing changes."
                                         : "Make a change before marking a new saved state.");
}

void AuthoringSummaryStrip::setCallbacks(SelectionSummaryCallbacks nextCallbacks)
{
    callbacks = std::move(nextCallbacks);
}
} // namespace drs::app::authoring
