#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace drs::app::authoring
{
// UI-thread state machine shared by Mixer, Instrument Controls, and the
// assignments drawer. Reserved transport/system controllers are intentionally
// ignored so Learn cannot steal sustain or panic semantics.
class MidiLearnCoordinator final
{
public:
    struct Assignment
    {
        int controllerNumber = 0;
        std::uint8_t channel = 0; // zero means any channel
    };

    void arm(std::string destinationId, std::uint64_t nowMs,
             std::uint64_t timeoutMs = 10000);
    void cancel() noexcept;
    bool isArmed() const noexcept { return armedDestination.has_value(); }
    const std::string& destinationId() const noexcept { return destination; }
    bool timedOut(std::uint64_t nowMs) const noexcept;
    std::optional<Assignment> observeCc(std::uint8_t channel,
                                        std::uint8_t controllerNumber,
                                        std::uint8_t value,
                                        std::uint64_t nowMs);

private:
    std::optional<std::string> armedDestination;
    std::string destination;
    std::uint64_t deadlineMs = 0;
};
} // namespace drs::app::authoring
