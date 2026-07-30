#pragma once

#include "drs/engine/ProjectRestoreCoordinator.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace drs::app
{
class HostStateRecoveryBanner final : public juce::Component
{
public:
    HostStateRecoveryBanner(std::function<void()> locateAction,
                            std::function<void()> retryAction);

    bool update(std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> snapshot);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

    std::uint64_t getDisplayedGeneration() const noexcept { return displayedGeneration; }
    juce::String getDisplayedStatusText() const { return statusLabel.getText(); }
    juce::String getDisplayedMessageText() const { return messageLabel.getText(); }

private:
    bool shouldPresent(const drs::engine::ProjectRestoreSnapshot& snapshot) const;
    bool canLocate(const drs::engine::ProjectRestoreSnapshot& snapshot) const;
    bool canRetry(const drs::engine::ProjectRestoreSnapshot& snapshot) const;
    void dismissCurrentGeneration();

    std::function<void()> locateAction;
    std::function<void()> retryAction;
    juce::Label projectLabel;
    juce::Label statusLabel;
    juce::Label messageLabel;
    juce::TextButton locateButton { "Locate" };
    juce::TextButton retryButton { "Retry" };
    juce::TextButton dismissButton { "Dismiss" };
    std::uint64_t displayedGeneration = 0;
    std::uint64_t dismissedGeneration = 0;
    drs::engine::ProjectRestoreState displayedState
        = drs::engine::ProjectRestoreState::idle;
};
} // namespace drs::app
