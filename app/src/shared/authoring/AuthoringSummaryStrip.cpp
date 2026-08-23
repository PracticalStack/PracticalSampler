#include "shared/authoring/AuthoringSummaryStrip.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

namespace drs::app::authoring
{
namespace
{
const auto summaryMuted = visual::textMuted;

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
                                "Summarizes the current zone selection and exposes preview and save actions.");

    titleLabel.setFont(juce::FontOptions(visual::titleTypeSize, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, visual::text);
    titleLabel.setComponentID("authoringSummaryTitleLabel");
    configureAccessibleMetadata(titleLabel,
                                "Selected zone title",
                                "Displays the selected zone name.");

    statusLabel.setColour(juce::Label::textColourId, summaryMuted);
    sourceLabel.setColour(juce::Label::textColourId, summaryMuted);
    articulationLabel.setColour(juce::Label::textColourId, summaryMuted);
    playbackLabel.setColour(juce::Label::textColourId, summaryMuted);
    statusLabel.setComponentID("authoringSummaryStatusLabel");
    sourceLabel.setComponentID("authoringSummarySourceLabel");
    articulationLabel.setComponentID("authoringSummaryArticulationLabel");
    playbackLabel.setComponentID("authoringSummaryPlaybackLabel");
    configureAccessibleMetadata(statusLabel,
                                "Selection status",
                                "Displays the current selection state.");
    configureAccessibleMetadata(sourceLabel,
                                "Selected zone source",
                                "Displays the selected zone sample source.");
    configureAccessibleMetadata(articulationLabel,
                                "Selected zone articulation",
                                "Displays the selected zone articulation.");
    configureAccessibleMetadata(playbackLabel,
                                "Playback revision state",
                                "Displays the current draft, preview, and published revision state.");

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

    prepareDraftButton.setButtonText("Prepare Draft");
    prepareDraftButton.setComponentID("authoringPrepareDraftButton");
    configureAccessibleMetadata(prepareDraftButton,
                                "Prepare draft playback",
                                "Builds the latest draft for playback preview.",
                                "Press to prepare the latest draft for playback preview.");
    prepareDraftButton.onClick = [this]
    {
        if (callbacks.onPrepareDraftPlaybackRequested)
            callbacks.onPrepareDraftPlaybackRequested();
    };

    publishDraftButton.setButtonText("Publish Draft");
    publishDraftButton.setComponentID("authoringPublishDraftButton");
    configureAccessibleMetadata(publishDraftButton,
                                "Publish draft playback",
                                "Publishes the latest prepared draft to the performance path.",
                                "Press to publish the latest prepared draft to the performance path.");
    publishDraftButton.onClick = [this]
    {
        if (callbacks.onPublishDraftPlaybackRequested)
            callbacks.onPublishDraftPlaybackRequested();
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
             static_cast<juce::Component*>(&playbackLabel),
             static_cast<juce::Component*>(&previewButton),
             static_cast<juce::Component*>(&prepareDraftButton),
             static_cast<juce::Component*>(&publishDraftButton),
             static_cast<juce::Component*>(&saveCheckpointButton)
         })
    {
        addAndMakeVisible(component);
    }
}

void AuthoringSummaryStrip::resized()
{
    auto hero = getLocalBounds();
    auto heroLeft = hero.removeFromLeft(hero.proportionOfWidth(0.54f));
    titleLabel.setBounds(heroLeft.removeFromTop(30));
    heroLeft.removeFromTop(2);
    statusLabel.setBounds(heroLeft.removeFromTop(20));
    heroLeft.removeFromTop(2);
    auto detailRow = heroLeft;
    constexpr auto detailGap = 6;
    const auto detailWidth = std::max(1, (detailRow.getWidth() - (detailGap * 2)) / 3);
    sourceLabel.setBounds(detailRow.removeFromLeft(detailWidth));
    detailRow.removeFromLeft(std::min(detailGap, detailRow.getWidth()));
    articulationLabel.setBounds(detailRow.removeFromLeft(detailWidth));
    detailRow.removeFromLeft(std::min(detailGap, detailRow.getWidth()));
    playbackLabel.setBounds(detailRow);

    auto heroButtons = hero.removeFromRight(350);
    auto topRow = heroButtons.removeFromTop(28);
    saveCheckpointButton.setBounds(topRow.removeFromLeft(120));
    heroButtons.removeFromTop(10);
    auto actionRow = heroButtons.removeFromTop(30);
    previewButton.setBounds(actionRow.removeFromLeft(140));
    actionRow.removeFromLeft(8);
    prepareDraftButton.setBounds(actionRow.removeFromLeft(94));
    actionRow.removeFromLeft(8);
    publishDraftButton.setBounds(actionRow.removeFromLeft(94));
}

void AuthoringSummaryStrip::setViewModel(SelectionSummaryViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    titleLabel.setText(juce::String::fromUTF8(viewModel.title.c_str()), juce::dontSendNotification);
    statusLabel.setText(juce::String::fromUTF8(viewModel.statusText.c_str()), juce::dontSendNotification);
    sourceLabel.setText(juce::String::fromUTF8(viewModel.sourceText.c_str()), juce::dontSendNotification);
    articulationLabel.setText(juce::String::fromUTF8(viewModel.articulationText.c_str()), juce::dontSendNotification);
    playbackLabel.setText(juce::String::fromUTF8(viewModel.playbackText.c_str()), juce::dontSendNotification);
    updateDynamicAccessibleText(titleLabel, titleLabel.getText(), "Selected zone title: ");
    updateDynamicAccessibleText(statusLabel, statusLabel.getText(), "Selection status: ");
    updateDynamicAccessibleText(sourceLabel, sourceLabel.getText(), "Selected zone source: ");
    updateDynamicAccessibleText(articulationLabel, articulationLabel.getText(), "Selected zone articulation: ");
    updateDynamicAccessibleText(playbackLabel, playbackLabel.getText(), "Playback revision state: ");

    previewButton.setEnabled(viewModel.canPreview);
    prepareDraftButton.setEnabled(viewModel.canPrepareDraftPlayback);
    publishDraftButton.setEnabled(viewModel.canPublishDraftPlayback);
    updateActionAccessibilityState(previewButton,
                                   viewModel.canPreview,
                                   "Previews the selected zone.",
                                   "Unavailable because no zone preview is available.",
                                   "Press to audition the selected zone.",
                                   "Select a zone to enable preview.");
    updateActionAccessibilityState(prepareDraftButton,
                                   viewModel.canPrepareDraftPlayback,
                                   "Builds the latest draft for playback preview.",
                                   "Unavailable because the current draft cannot be prepared for playback yet.",
                                   "Press to prepare the latest draft for playback preview.",
                                   "Import at least one playable zone to enable draft preparation.");
    updateActionAccessibilityState(publishDraftButton,
                                   viewModel.canPublishDraftPlayback,
                                   "Publishes the latest prepared draft to the performance path.",
                                   "Unavailable because the latest draft is not ready to publish yet.",
                                   "Press to publish the latest prepared draft to the performance path.",
                                   "Prepare the latest draft before publishing it to the performance path.");
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
