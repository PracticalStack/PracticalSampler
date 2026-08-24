#pragma once

#include "drs/engine/InstrumentControlContract.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeInstrumentControlBindingIssue
{
    std::string code;
    std::string detail;
};

// Immutable after compilation. The audio callback performs only bounded array
// lookups; all string/ID resolution and conflict checks happen off-thread.
class InstrumentControlBindingTable final
{
public:
    static constexpr std::size_t invalidControlIndex = maximumInstrumentControls;

    bool compile(const std::vector<RuntimeProjectInstrumentControlDefinition>& controls,
                 const std::vector<RuntimeProjectMidiControlBindingDefinition>& bindings,
                 std::vector<RuntimeInstrumentControlBindingIssue>& issues);

    std::size_t controlCount() const noexcept { return controlCountValue; }
    std::size_t resolve(std::uint8_t midiChannel1Based,
                        std::uint8_t controllerNumber) const noexcept;
    double defaultValue(std::size_t controlIndex) const noexcept;
    int destinationController(std::size_t controlIndex) const noexcept;
    const std::string& controlId(std::size_t controlIndex) const noexcept;

private:
    std::size_t controlCountValue = 0;
    std::array<std::size_t, 128> anyChannel {};
    std::array<std::array<std::size_t, 16>, 128> exactChannel {};
    std::array<double, maximumInstrumentControls> defaults {};
    std::array<int, maximumInstrumentControls> destinationControllers {};
    std::array<std::string, maximumInstrumentControls> ids {};
};

// Shared normalized state for UI, MIDI, restore, and reset. Atomic values make
// reads/writes safe across message and audio threads; structural changes still
// require a newly compiled table.
class InstrumentControlRuntimeState final
{
public:
    InstrumentControlRuntimeState() noexcept;

    void prepare(const InstrumentControlBindingTable& table) noexcept;
    void resetAll() noexcept;
    bool resetControl(std::size_t controlIndex) noexcept;
    bool setControlNormalized(std::size_t controlIndex, double normalized) noexcept;
    bool applyMidi(std::uint8_t midiChannel1Based,
                   std::uint8_t controllerNumber,
                   std::uint8_t value,
                   const InstrumentControlBindingTable& table) noexcept;
    double currentValue(std::size_t controlIndex) const noexcept;
    double defaultValue(std::size_t controlIndex) const noexcept;
    std::uint64_t generation() const noexcept { return valueGeneration.load(std::memory_order_acquire); }

private:
    std::array<std::atomic<double>, maximumInstrumentControls> values;
    std::array<double, maximumInstrumentControls> defaults {};
    std::size_t controlCountValue = 0;
    std::atomic<std::uint64_t> valueGeneration { 0 };
};
} // namespace drs::engine
