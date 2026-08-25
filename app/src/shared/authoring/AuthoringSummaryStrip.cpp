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
                                "Summarizes the current zone selection and exposes preview and playback actions.");

    statusLabel.setColour(juce::Label::textColourId, summaryMuted);
    sourceLabel.setColour(juce::Label::textColourId, summaryMuted);
    articulationLabel.setColour(juce::Label::textColourId, summaryMuted);
    playbackLabel.setColour(juce::Label::textColourId, summaryMuted);
    for (auto* label : { &statusLabel, &sourceLabel, &articulationLabel, &playbackLabel })
    {
        label->setFont(juce::FontOptions(visual::compactTypeSize, juce::Font::plain));
        label->setMinimumHorizontalScale(0.68f);
        label->setJustificationType(juce::Justification::centredLeft);
    }
    statusLabel.setFont(juce::FontOptions(visual::compactTypeSize, juce::Font::bold));
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

    previewButton.setButtonText("Preview Zone");
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

    for (auto* component : {
             static_cast<juce::Component*>(&statusLabel),
             static_cast<juce::Component*>(&sourceLabel),
             static_cast<juce::Component*>(&articulationLabel),
             static_cast<juce::Component*>(&playbackLabel),
             static_cast<juce::Component*>(&previewButton),
             static_cast<juce::Component*>(&prepareDraftButton),
             static_cast<juce::Component*>(&publishDraftButton)
         })
    {
        addAndMakeVisible(component);
    }
}

void AuthoringSummaryStrip::paint(juce::Graphics& g)
{
    const auto surface = getLocalBounds().toFloat().reduced(0.5f, 4.5f);
    g.setColour(visual::surfaceSubtle);
    g.fillRoundedRectangle(surface, visual::panelRadius);
    g.setColour(visual::border);
    g.drawRoundedRectangle(surface, visual::panelRadius, visual::borderWidth);

    g.setColour(visual::border.withAlpha(0.7f));
    const auto top = surface.getY() + 10.0f;
    const auto bottom = surface.getBottom() - 10.0f;
    for (const auto* label : { &statusLabel, &sourceLabel, &articulationLabel })
    {
        const auto dividerX = static_cast<float>(label->getRight() + 3);
        if (dividerX < static_cast<float>(previewButton.getX() - 8))
            g.drawVerticalLine(juce::roundToInt(dividerX), top, bottom);
    }
    const auto actionDividerX = previewButton.getX() - 10;
    if (actionDividerX > playbackLabel.getX())
        g.drawVerticalLine(actionDividerX, top, bottom);
}

void AuthoringSummaryStrip::resized()
{
    auto area = getLocalBounds().reduced(12, 14);
    auto actionArea = area.removeFromRight(294);
    constexpr auto detailGap = 8;
    const auto diagnosticWidth = area.getWidth();
    const auto statusWidth = std::min(152, std::max(108, diagnosticWidth * 20 / 100));
    const auto sourceWidth = std::min(210, std::max(118, diagnosticWidth * 26 / 100));
    const auto articulationWidth = std::min(176, std::max(106, diagnosticWidth * 22 / 100));
    statusLabel.setBounds(area.removeFromLeft(statusWidth));
    area.removeFromLeft(detailGap);
    sourceLabel.setBounds(area.removeFromLeft(sourceWidth));
    area.removeFromLeft(detailGap);
    articulationLabel.setBounds(area.removeFromLeft(articulationWidth));
    area.removeFromLeft(detailGap);
    playbackLabel.setBounds(area);

    auto actionRow = actionArea;
    previewButton.setBounds(actionRow.removeFromLeft(112));
    actionRow.removeFromLeft(8);
    prepareDraftButton.setBounds(actionRow.removeFromLeft(82));
    actionRow.removeFromLeft(8);
    publishDraftButton.setBounds(actionRow.removeFromLeft(82));
}

void AuthoringSummaryStrip::setViewModel(SelectionSummaryViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    statusLabel.setText(juce::String::fromUTF8(viewModel.statusText.c_str()), juce::dontSendNotification);
    sourceLabel.setText(juce::String::fromUTF8(viewModel.sourceText.c_str()), juce::dontSendNotification);
    articulationLabel.setText(juce::String::fromUTF8(viewModel.articulationText.c_str()), juce::dontSendNotification);
    playbackLabel.setText(juce::String::fromUTF8(viewModel.playbackText.c_str()), juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          statusLabel.getText().startsWith("Dirty")
                              ? visual::warning : visual::success);
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
}

void AuthoringSummaryStrip::setCallbacks(SelectionSummaryCallbacks nextCallbacks)
{
    callbacks = std::move(nextCallbacks);
}
} // namespace drs::app::authoring
