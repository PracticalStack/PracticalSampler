#include "shared/StatusPanel.h"

namespace drs::app
{
namespace
{
juce::String toBulletList(const std::vector<std::string>& lines)
{
    juce::String result;

    for (const auto& line : lines)
        result << juce::String::fromUTF8(line.c_str()) << "\n";

    return result.trimEnd();
}
} // namespace

StatusPanel::StatusPanel(drs::engine::EngineFacade& facade)
    : engineFacade(facade)
{
    titleLabel.setText("Engine Status", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));

    modeLabel.setJustificationType(juce::Justification::centredLeft);
    stateLabel.setJustificationType(juce::Justification::centredLeft);

    detailEditor.setMultiLine(true);
    detailEditor.setReadOnly(true);
    detailEditor.setScrollbarsShown(true);
    detailEditor.setCaretVisible(false);
    detailEditor.setPopupMenuEnabled(false);

    nextStepsLabel.setText("Current next steps", juce::dontSendNotification);
    nextStepsLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

    nextStepsEditor.setMultiLine(true);
    nextStepsEditor.setReadOnly(true);
    nextStepsEditor.setScrollbarsShown(true);
    nextStepsEditor.setCaretVisible(false);
    nextStepsEditor.setPopupMenuEnabled(false);

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(modeLabel);
    addAndMakeVisible(stateLabel);
    addAndMakeVisible(detailEditor);
    addAndMakeVisible(nextStepsLabel);
    addAndMakeVisible(nextStepsEditor);

    refreshSnapshot();
    startTimerHz(2);
}

void StatusPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(juce::Colour::fromRGB(18, 22, 28));

    g.setColour(juce::Colour::fromRGB(47, 84, 235));
    g.fillRoundedRectangle(bounds.reduced(12.0f), 18.0f);

    g.setColour(juce::Colour::fromRGB(247, 249, 252));
    g.fillRoundedRectangle(bounds.reduced(16.0f), 14.0f);
}

void StatusPanel::resized()
{
    auto area = getLocalBounds().reduced(32);

    titleLabel.setBounds(area.removeFromTop(32));
    area.removeFromTop(8);
    modeLabel.setBounds(area.removeFromTop(24));
    stateLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(12);
    detailEditor.setBounds(area.removeFromTop(130));
    area.removeFromTop(12);
    nextStepsLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    nextStepsEditor.setBounds(area);
}

void StatusPanel::timerCallback()
{
    refreshSnapshot();
}

void StatusPanel::refreshSnapshot()
{
    snapshot = engineFacade.getStatusSnapshot();

    modeLabel.setText("Mode: " + juce::String::fromUTF8(snapshot.mode.c_str()), juce::dontSendNotification);
    stateLabel.setText("State: " + juce::String::fromUTF8(snapshot.integrationState.c_str()), juce::dontSendNotification);
    detailEditor.setText(juce::String::fromUTF8(snapshot.detail.c_str()), false);
    nextStepsEditor.setText(toBulletList(snapshot.nextSteps), false);
}
} // namespace drs::app
