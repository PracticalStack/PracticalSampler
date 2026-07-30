#include "shared/HostStateRecoveryBanner.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::shared_ptr<const drs::engine::ProjectRestoreSnapshot> makeSnapshot(
    const std::uint64_t generation,
    const drs::engine::ProjectRestoreState state,
    const drs::engine::ProjectRestoreFinding finding,
    const std::string& message)
{
    auto snapshot = std::make_shared<drs::engine::ProjectRestoreSnapshot>();
    snapshot->generation = generation;
    snapshot->state = state;
    snapshot->finding = finding;
    snapshot->expectedProjectId = "drs.test.authored-project";
    snapshot->message = message;
    drs::engine::HostSessionState hostState;
    hostState.projectBinding.manifestFileName = "authored-project.drsproj";
    snapshot->hostState = std::move(hostState);
    return snapshot;
}

juce::TextButton& findButton(juce::Component& component, const juce::String& id)
{
    auto* button = dynamic_cast<juce::TextButton*>(component.findChildWithID(id));
    if (button == nullptr)
        throw std::runtime_error("Recovery banner button '" + id.toStdString() + "' was not found.");
    return *button;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        int locateCount = 0;
        int retryCount = 0;
        drs::app::HostStateRecoveryBanner banner(
            [&] { ++locateCount; },
            [&] { ++retryCount; });
        banner.setBounds(0, 0, 900, 42);

        require(!banner.isVisible(),
                "Recovery banner must start hidden for an idle coordinator.");

        const auto wrongIdentity = makeSnapshot(
            1,
            drs::engine::ProjectRestoreState::needsLocation,
            drs::engine::ProjectRestoreFinding::identityMismatch,
            "The selected file belongs to another project.");
        banner.update(wrongIdentity);
        require(banner.isVisible()
                    && banner.getDisplayedStatusText().contains("NeedsLocation")
                    && banner.getDisplayedStatusText().contains("IdentityMismatch")
                    && banner.getDisplayedMessageText().contains("another project"),
                "Wrong-ID recovery must be visible and explicitly distinguished.");
        require(findButton(banner, "hostStateRecoveryLocate").isVisible()
                    && findButton(banner, "hostStateRecoveryRetry").isVisible(),
                "Needs Location must expose Locate and Retry actions.");

        findButton(banner, "hostStateRecoveryLocate").onClick();
        findButton(banner, "hostStateRecoveryRetry").onClick();
        require(locateCount == 1 && retryCount == 1,
                "Recovery actions must route through shell-provided callbacks.");

        const auto changedContent = makeSnapshot(
            2,
            drs::engine::ProjectRestoreState::needsLocation,
            drs::engine::ProjectRestoreFinding::contentChanged,
            "The project ID matches but canonical content changed.");
        banner.update(changedContent);
        require(banner.getDisplayedStatusText().contains("ContentChanged")
                    && !banner.getDisplayedStatusText().contains("IdentityMismatch"),
                "Changed content must remain distinct from a wrong project identity.");

        findButton(banner, "hostStateRecoveryDismiss").onClick();
        require(!banner.isVisible(),
                "Dismiss must hide the current non-modal recovery notice.");
        banner.update(changedContent);
        require(!banner.isVisible(),
                "A dismissed generation must stay hidden until state generation changes.");

        const auto legacy = makeSnapshot(
            3,
            drs::engine::ProjectRestoreState::active,
            drs::engine::ProjectRestoreFinding::legacyUnboundProject,
            "Legacy preset restored without inferring a project.");
        banner.update(legacy);
        require(banner.isVisible()
                    && banner.getDisplayedStatusText().contains("LegacyUnboundProject")
                    && !findButton(banner, "hostStateRecoveryLocate").isVisible(),
                "Legacy migration must appear as a non-modal notice without a misleading Locate action.");

        const auto degraded = makeSnapshot(
            4,
            drs::engine::ProjectRestoreState::degraded,
            drs::engine::ProjectRestoreFinding::projectLoadFailed,
            "Some samples require repair.");
        banner.update(degraded);
        require(banner.isVisible()
                    && findButton(banner, "hostStateRecoveryRetry").isVisible(),
                "Degraded content must show a non-modal retry notice.");

        const auto active = makeSnapshot(
            5,
            drs::engine::ProjectRestoreState::active,
            drs::engine::ProjectRestoreFinding::none,
            "Exact restored content active.");
        banner.update(active);
        require(!banner.isVisible(),
                "A healthy exact activation must not consume persistent screen space.");

        std::cout << "Host-state recovery UI tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Host-state recovery UI tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
