#include "drs/engine/PerformanceLaneState.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
bool PerformanceActionScratch::push(const SamplerRenderEvent event) noexcept
{
    if (count >= events.size()) return false;
    events[count++] = event;
    return true;
}

std::size_t PerformanceLaneState::heldIndex(const std::uint8_t channel, const std::uint8_t note) noexcept
{
    return static_cast<std::size_t>(channel & 0x0fu) * noteCount + static_cast<std::size_t>(note);
}

std::uint8_t PerformanceLaneState::toMidiVelocity(const float value) noexcept
{
    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(value * 127.0f)), 0, 127));
}

void PerformanceLaneState::reset() noexcept
{
    heldNotes = {};
    selectedArticulationIndex = kInvalidPerformanceProgramIndex;
    selectedArticulationStableId = 0;
    articulationCount = 0;
    pedalIsDown = false;
    actionOverflowCount = 0;
    semanticEventCounts = {};
}

void PerformanceLaneState::migrateProgram(const CompiledPerformanceProgram& program,
                                          const std::uint64_t activationGeneration) noexcept
{
    std::uint32_t migrated = kInvalidPerformanceProgramIndex;
    if (selectedArticulationStableId != 0)
    {
        for (std::size_t index = 0; index < program.articulationStableIds.size(); ++index)
            if (program.articulationStableIds[index] == selectedArticulationStableId)
            {
                migrated = static_cast<std::uint32_t>(index);
                break;
            }
    }
    if (migrated == kInvalidPerformanceProgramIndex
        && program.defaultArticulationIndex < program.articulationCount)
    {
        migrated = program.defaultArticulationIndex;
    }
    selectedArticulationIndex = migrated;
    articulationCount = program.articulationCount;
    selectedArticulationStableId = migrated < program.articulationStableIds.size()
        ? program.articulationStableIds[migrated] : 0;
    // Existing records deliberately retain their original generation and articulation.
    (void) activationGeneration;
}

void PerformanceLaneState::recordNoteOn(const SamplerRenderEvent& event,
                                        const std::uint64_t generation,
                                        const bool consumed) noexcept
{
    auto& held = heldNotes[heldIndex(event.midiChannel, event.midiNote)];
    held = {};
    held.active = true;
    held.physicalKeyDown = true;
    held.attackVelocity = toMidiVelocity(event.velocity);
    held.midiChannel = event.midiChannel & 0x0fu;
    held.midiNote = event.midiNote;
    held.articulationAtAttack = selectedArticulationIndex;
    held.activationGeneration = generation;
    held.consumed = consumed;
}

void PerformanceLaneState::recordNoteOff(const SamplerRenderEvent& event) noexcept
{
    auto& held = heldNotes[heldIndex(event.midiChannel, event.midiNote)];
    if (!held.active) return;
    held.physicalKeyDown = false;
    held.noteOffVelocity = toMidiVelocity(event.noteOffVelocity > 0.0f
                                               ? event.noteOffVelocity : event.velocity);
    held.pedalDeferred = pedalIsDown;
}

void PerformanceLaneState::setPedal(const bool down) noexcept
{
    pedalIsDown = down;
    if (!down)
        for (auto& held : heldNotes)
            if (held.active && !held.physicalKeyDown)
                held.pedalDeferred = false;
}

bool PerformanceLaneState::normalize(const SamplerRenderEvent& raw,
                                     const std::uint64_t activationGeneration,
                                     const CompiledPerformanceProgram& program,
                                     PerformanceActionScratch& scratch) noexcept
{
    // Preflight reserves the entire action set before mutating lane state.
    if (scratch.size() >= PerformanceActionScratch::capacity)
    {
        ++actionOverflowCount;
        return false;
    }
    auto event = raw;
    switch (raw.type)
    {
        case SamplerRenderEventType::noteOn:
        {
            const auto activation = program.activationByMidiNote[raw.midiNote];
            const auto isActivation = activation.articulationIndex < program.articulationCount;
            if (isActivation)
            {
                selectedArticulationIndex = activation.articulationIndex;
                selectedArticulationStableId = activation.articulationIndex < program.articulationStableIds.size()
                    ? program.articulationStableIds[activation.articulationIndex] : 0;
            }
            recordNoteOn(event, activationGeneration, isActivation && activation.consume);
            ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOn)];
            if (isActivation && activation.consume) return true;
            event.articulationIndex = selectedArticulationIndex;
            return scratch.push(event);
        }
        case SamplerRenderEventType::noteOff:
        {
            const auto consumed = heldNotes[heldIndex(event.midiChannel, event.midiNote)].active
                && heldNotes[heldIndex(event.midiChannel, event.midiNote)].consumed;
            recordNoteOff(event);
            ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOff)];
            if (consumed)
            {
                heldNotes[heldIndex(event.midiChannel, event.midiNote)] = {};
                return true;
            }
            event.articulationIndex = heldNotes[heldIndex(event.midiChannel, event.midiNote)].articulationAtAttack;
            return scratch.push(event);
        }
        case SamplerRenderEventType::sustainPedal:
        {
            const auto down = event.velocity >= (64.0f / 127.0f);
            if (down == pedalIsDown) return true;
            setPedal(down);
            event.type = down ? SamplerRenderEventType::pedalDown : SamplerRenderEventType::pedalUp;
            ++semanticEventCounts[static_cast<std::size_t>(down ? PerformanceEventKind::pedalDown
                                                                  : PerformanceEventKind::pedalUp)];
            return scratch.push(event);
        }
        case SamplerRenderEventType::pedalDown:
        case SamplerRenderEventType::pedalUp:
        {
            const auto down = raw.type == SamplerRenderEventType::pedalDown;
            if (down == pedalIsDown) return true;
            setPedal(down);
            ++semanticEventCounts[static_cast<std::size_t>(down ? PerformanceEventKind::pedalDown
                                                                  : PerformanceEventKind::pedalUp)];
            return scratch.push(event);
        }
        case SamplerRenderEventType::allNotesOff:
            for (auto& held : heldNotes) if (held.active) held.physicalKeyDown = false;
            return scratch.push(event);
        case SamplerRenderEventType::reset:
            heldNotes = {};
            pedalIsDown = false;
            return scratch.push(event);
    }
    return false;
}

PerformanceLaneStateSnapshot PerformanceLaneState::getSnapshot() const noexcept
{
    PerformanceLaneStateSnapshot result;
    result.selectedArticulationIndex = selectedArticulationIndex;
    result.pedalDown = pedalIsDown;
    result.actionOverflowCount = actionOverflowCount;
    result.semanticEventCounts = semanticEventCounts;
    for (const auto& held : heldNotes)
    {
        result.heldNoteCount += held.active ? 1u : 0u;
        result.consumedNoteCount += held.active && held.consumed ? 1u : 0u;
    }
    return result;
}

PerformanceHeldNoteRecord PerformanceLaneState::getHeldNote(const std::uint8_t channel,
                                                             const std::uint8_t note) const noexcept
{
    return heldNotes[heldIndex(channel, note)];
}
} // namespace drs::engine
