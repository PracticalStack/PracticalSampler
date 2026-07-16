#include "shared/authoring/AuthoringSummaryStrip.h"

namespace drs::app::authoring
{
namespace
{
const auto summaryMuted = juce::Colour::fromRGB(82, 86, 94);
} // namespace

AuthoringSummaryStrip::AuthoringSummaryStrip()
{
    setComponentID("authoringSummaryStrip");

    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    statusLabel.setColour(juce::Label::textColourId, summaryMuted);
    sourceLabel.setColour(juce::Label::textColourId, summaryMuted);
    articulationLabel.setColour(juce::Label::textColourId, summaryMuted);

    previewButton.setButtonText("Preview Selected Zone");
    previewButton.setComponentID("authoringPreviewButton");
    previewButton.onClick = [this]
    {
        if (callbacks.onPreviewRequested)
            callbacks.onPreviewRequested();
    };

    undoButton.setButtonText("Undo");
    undoButton.setComponentID("authoringUndoButton");
    undoButton.onClick = [this]
    {
        if (callbacks.onUndoRequested)
            callbacks.onUndoRequested();
    };

    redoButton.setButtonText("Redo");
    redoButton.setComponentID("authoringRedoButton");
    redoButton.onClick = [this]
    {
        if (callbacks.onRedoRequested)
            callbacks.onRedoRequested();
    };

    saveCheckpointButton.setButtonText("Mark Saved");
    saveCheckpointButton.setComponentID("authoringSaveButton");
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

    previewButton.setEnabled(viewModel.canPreview);
    undoButton.setEnabled(viewModel.canUndo);
    redoButton.setEnabled(viewModel.canRedo);
}

void AuthoringSummaryStrip::setCallbacks(SelectionSummaryCallbacks nextCallbacks)
{
    callbacks = std::move(nextCallbacks);
}
} // namespace drs::app::authoring
