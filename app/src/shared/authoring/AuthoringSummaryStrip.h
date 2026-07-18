#pragma once

#include "shared/authoring/AuthoringViewModels.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class AuthoringSummaryStrip final : public juce::Component
{
public:
    AuthoringSummaryStrip();

    void resized() override;
    void setViewModel(SelectionSummaryViewModel nextViewModel);
    void setCallbacks(SelectionSummaryCallbacks nextCallbacks);

private:
    SelectionSummaryViewModel viewModel;
    SelectionSummaryCallbacks callbacks;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label sourceLabel;
    juce::Label articulationLabel;
    juce::Label playbackLabel;
    juce::TextButton previewButton;
    juce::TextButton prepareDraftButton;
    juce::TextButton publishDraftButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton saveCheckpointButton;
};
} // namespace drs::app::authoring
