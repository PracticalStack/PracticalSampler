#include "shared/HostStateRecoveryBanner.h"

#include <utility>

namespace drs::app
{
HostStateRecoveryBanner::HostStateRecoveryBanner(std::function<void()> onLocate,
                                                 std::function<void()> onRetry)
    : locateAction(std::move(onLocate)),
      retryAction(std::move(onRetry))
{
    setComponentID("hostStateRecoveryBanner");
    setVisible(false);

    projectLabel.setComponentID("hostStateRecoveryProject");
    projectLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    projectLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(projectLabel);

    statusLabel.setComponentID("hostStateRecoveryStatus");
    statusLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    messageLabel.setComponentID("hostStateRecoveryMessage");
    messageLabel.setFont(juce::FontOptions(12.0f));
    messageLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(messageLabel);

    locateButton.setComponentID("hostStateRecoveryLocate");
    locateButton.onClick = [this]
    {
        if (locateAction)
            locateAction();
    };
    addAndMakeVisible(locateButton);

    retryButton.setComponentID("hostStateRecoveryRetry");
    retryButton.onClick = [this]
    {
        if (retryAction)
            retryAction();
    };
    addAndMakeVisible(retryButton);

    dismissButton.setComponentID("hostStateRecoveryDismiss");
    dismissButton.onClick = [this] { dismissCurrentGeneration(); };
    addAndMakeVisible(dismissButton);
}

bool HostStateRecoveryBanner::update(
    std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> snapshot)
{
    const auto wasVisible = isVisible();
    if (snapshot == nullptr || snapshot->generation == 0
        || snapshot->generation == dismissedGeneration
        || !shouldPresent(*snapshot))
    {
        setVisible(false);
        return wasVisible != isVisible();
    }

    displayedGeneration = snapshot->generation;
    displayedState = snapshot->state;

    juce::String projectName;
    if (snapshot->hostState.has_value())
        projectName = snapshot->hostState->projectBinding.manifestFileName;
    if (projectName.isEmpty())
        projectName = juce::String::fromUTF8(snapshot->expectedProjectId.c_str());
    if (projectName.isEmpty())
        projectName = "DAW state";
    projectLabel.setText(projectName, juce::dontSendNotification);

    auto status = juce::String(drs::engine::toString(snapshot->state));
    if (snapshot->finding != drs::engine::ProjectRestoreFinding::none)
        status += " · " + juce::String(drs::engine::toString(snapshot->finding));
    statusLabel.setText(status, juce::dontSendNotification);

    const auto message = juce::String::fromUTF8(snapshot->message.c_str());
    messageLabel.setText(message, juce::dontSendNotification);
    messageLabel.setTooltip(message);
    locateButton.setVisible(canLocate(*snapshot));
    retryButton.setVisible(canRetry(*snapshot));
    dismissButton.setVisible(true);
    setVisible(true);
    resized();
    return wasVisible != isVisible();
}

void HostStateRecoveryBanner::paint(juce::Graphics& graphics)
{
    const auto warning = displayedState == drs::engine::ProjectRestoreState::needsLocation
        || displayedState == drs::engine::ProjectRestoreState::degraded;
    const auto failed = displayedState == drs::engine::ProjectRestoreState::failed;
    const auto background = failed
        ? juce::Colour::fromRGB(82, 31, 35)
        : (warning ? juce::Colour::fromRGB(83, 60, 24)
                   : juce::Colour::fromRGB(30, 58, 79));
    graphics.fillAll(background);
    graphics.setColour(juce::Colours::white.withAlpha(0.16f));
    graphics.drawLine(0.0f,
                      static_cast<float>(getHeight() - 1),
                      static_cast<float>(getWidth()),
                      static_cast<float>(getHeight() - 1));
}

void HostStateRecoveryBanner::resized()
{
    auto area = getLocalBounds().reduced(10, 5);
    auto actions = area.removeFromRight(218);
    dismissButton.setBounds(actions.removeFromRight(68));
    actions.removeFromRight(6);
    retryButton.setBounds(actions.removeFromRight(62));
    actions.removeFromRight(6);
    locateButton.setBounds(actions.removeFromRight(70));

    projectLabel.setBounds(area.removeFromLeft(180));
    statusLabel.setBounds(area.removeFromLeft(210));
    messageLabel.setBounds(area);
}

bool HostStateRecoveryBanner::shouldPresent(
    const drs::engine::ProjectRestoreSnapshot& snapshot) const
{
    if (snapshot.state == drs::engine::ProjectRestoreState::idle)
        return false;

    if (snapshot.state == drs::engine::ProjectRestoreState::active
        && snapshot.finding == drs::engine::ProjectRestoreFinding::none)
        return false;

    if (snapshot.state == drs::engine::ProjectRestoreState::ready
        && snapshot.finding == drs::engine::ProjectRestoreFinding::none)
        return false;

    return true;
}

bool HostStateRecoveryBanner::canLocate(
    const drs::engine::ProjectRestoreSnapshot& snapshot) const
{
    return snapshot.state == drs::engine::ProjectRestoreState::needsLocation
        && (snapshot.finding == drs::engine::ProjectRestoreFinding::candidateMissing
            || snapshot.finding == drs::engine::ProjectRestoreFinding::identityMismatch
            || snapshot.finding == drs::engine::ProjectRestoreFinding::contentChanged
            || snapshot.finding == drs::engine::ProjectRestoreFinding::projectLoadFailed);
}

bool HostStateRecoveryBanner::canRetry(
    const drs::engine::ProjectRestoreSnapshot& snapshot) const
{
    return snapshot.state == drs::engine::ProjectRestoreState::needsLocation
        || snapshot.state == drs::engine::ProjectRestoreState::failed
        || snapshot.state == drs::engine::ProjectRestoreState::degraded;
}

void HostStateRecoveryBanner::dismissCurrentGeneration()
{
    dismissedGeneration = displayedGeneration;
    setVisible(false);
}
} // namespace drs::app
