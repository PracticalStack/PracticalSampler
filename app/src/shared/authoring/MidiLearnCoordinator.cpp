#include "shared/authoring/MidiLearnCoordinator.h"

#include <algorithm>

namespace drs::app::authoring
{
void MidiLearnCoordinator::arm(std::string destinationId,
                               const std::uint64_t nowMs,
                               const std::uint64_t timeoutMs)
{
    destination = std::move(destinationId);
    armedDestination = destination;
    deadlineMs = nowMs + std::max<std::uint64_t>(1, timeoutMs);
}

void MidiLearnCoordinator::cancel() noexcept
{
    armedDestination.reset();
    destination.clear();
    deadlineMs = 0;
}

bool MidiLearnCoordinator::timedOut(const std::uint64_t nowMs) const noexcept
{
    return armedDestination.has_value() && nowMs >= deadlineMs;
}

std::optional<MidiLearnCoordinator::Assignment> MidiLearnCoordinator::observeCc(
    const std::uint8_t channel,
    const std::uint8_t controllerNumber,
    const std::uint8_t /*value*/,
    const std::uint64_t nowMs)
{
    if (!armedDestination.has_value() || timedOut(nowMs)
        || controllerNumber == 64 || controllerNumber == 120 || controllerNumber == 123)
    {
        if (timedOut(nowMs))
            cancel();
        return std::nullopt;
    }
    const auto assignment = Assignment { static_cast<int>(controllerNumber), channel };
    cancel();
    return assignment;
}
} // namespace drs::app::authoring
