#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace drs::engine
{
struct PerformanceHeldNoteRecord
{
    bool active = false;
    bool physicalKeyDown = false;
    bool pedalDeferred = false;
    bool releaseEmitted = false;
    bool consumed = false;
    std::uint8_t attackVelocity = 0;
    std::uint8_t noteOffVelocity = 0;
    std::uint8_t midiChannel = 0;
    std::uint8_t midiNote = 0;
    std::uint32_t articulationAtAttack = kInvalidPerformanceProgramIndex;
    std::uint64_t activationGeneration = 0;
};

struct PerformanceLaneStateSnapshot
{
    std::uint32_t selectedArticulationIndex = kInvalidPerformanceProgramIndex;
    bool pedalDown = false;
    std::uint32_t heldNoteCount = 0;
    std::uint32_t consumedNoteCount = 0;
    std::uint64_t actionOverflowCount = 0;
    std::array<std::uint64_t, 5> semanticEventCounts {};
};

// Fixed action storage makes one normalized event all-or-nothing. Future trigger actions use
// this same scratch; Sprint 4 only emits one forwarded semantic event per accepted input.
class PerformanceActionScratch final
{
public:
    static constexpr std::size_t capacity = 128;
    bool push(SamplerRenderEvent event) noexcept;
    void clear() noexcept { count = 0; }
    SamplerRenderEventView view() const noexcept { return { events.data(), count }; }
    std::size_t size() const noexcept { return count; }

private:
    std::array<SamplerRenderEvent, capacity> events {};
    std::size_t count = 0;
};

class PerformanceLaneState final
{
public:
    static constexpr std::size_t channelCount = 16;
    static constexpr std::size_t noteCount = 128;

    void reset() noexcept;
    void migrateProgram(const CompiledPerformanceProgram& program,
                        std::uint64_t activationGeneration) noexcept;
    bool normalize(const SamplerRenderEvent& raw,
                   std::uint64_t activationGeneration,
                   const CompiledPerformanceProgram& program,
                   PerformanceActionScratch& scratch) noexcept;
    PerformanceLaneStateSnapshot getSnapshot() const noexcept;
    PerformanceHeldNoteRecord getHeldNote(std::uint8_t channel, std::uint8_t note) const noexcept;

private:
    static std::size_t heldIndex(std::uint8_t channel, std::uint8_t note) noexcept;
    static std::uint8_t toMidiVelocity(float value) noexcept;
    void recordNoteOn(const SamplerRenderEvent& event, std::uint64_t generation, bool consumed) noexcept;
    void recordNoteOff(const SamplerRenderEvent& event) noexcept;
    void setPedal(bool down) noexcept;

    std::array<PerformanceHeldNoteRecord, channelCount * noteCount> heldNotes {};
    std::uint32_t selectedArticulationIndex = kInvalidPerformanceProgramIndex;
    std::uint64_t selectedArticulationStableId = 0;
    std::uint32_t articulationCount = 0;
    bool pedalIsDown = false;
    std::uint64_t actionOverflowCount = 0;
    std::array<std::uint64_t, 5> semanticEventCounts {};
};
} // namespace drs::engine
