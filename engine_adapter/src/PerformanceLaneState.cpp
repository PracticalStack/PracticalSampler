#include "drs/engine/PerformanceLaneState.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
namespace
{
bool hasTriggerRoute(const CompiledPerformanceProgram& program,
                     const PerformanceEventKind event,
                     const std::uint32_t articulationIndex,
                     const bool pedalDown) noexcept
{
    const auto eventIndex = static_cast<std::size_t>(event);
    if (eventIndex >= program.eventRanges.size()) return false;
    const auto& range = program.eventRanges[eventIndex];
    const auto first = std::min<std::size_t>(range.firstRoute, program.triggerRoutes.size());
    const auto last = std::min<std::size_t>(first + range.routeCount, program.triggerRoutes.size());
    for (std::size_t index = first; index < last; ++index)
    {
        const auto& route = program.triggerRoutes[index];
        if (route.articulationIndex != articulationIndex) continue;
        if (route.sustain == PerformanceSustainCondition::any
            || (route.sustain == PerformanceSustainCondition::pedalDown && pedalDown)
            || (route.sustain == PerformanceSustainCondition::pedalUp && !pedalDown))
            return true;
    }
    return false;
}

bool hasRoundRobinResetRule(const CompiledPerformanceProgram& program,
                            const RoundRobinResetEvent event) noexcept
{
    for (const auto& rule : program.roundRobinResets)
        if (rule.event == event)
            return true;
    return false;
}
} // namespace

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
    if (held.noteOffVelocity == 0)
        held.noteOffVelocity = held.attackVelocity;
    held.pedalDeferred = pedalIsDown;
}

void PerformanceLaneState::setPedal(const bool down) noexcept
{
    pedalIsDown = down;
}

SamplerRenderEvent PerformanceLaneState::makeTriggerEvent(const SamplerRenderEvent& source,
                                                           const PerformanceHeldNoteRecord& held,
                                                           const PerformanceEventKind kind,
                                                           const bool pedalDown) noexcept
{
    SamplerRenderEvent result = source;
    result.type = SamplerRenderEventType::noteOn;
    result.midiChannel = held.midiChannel;
    result.midiNote = held.midiNote;
    result.velocity = static_cast<float>(held.noteOffVelocity) / 127.0f;
    result.noteOffVelocity = static_cast<float>(held.noteOffVelocity) / 127.0f;
    result.articulationIndex = held.articulationAtAttack;
    result.performanceEvent = kind;
    result.sustainPedalDown = pedalDown;
    return result;
}

SamplerRenderEvent PerformanceLaneState::makePedalTriggerEvent(const SamplerRenderEvent& source,
                                                                const std::uint32_t articulationIndex,
                                                                const PerformanceEventKind kind,
                                                                const bool pedalDown) noexcept
{
    SamplerRenderEvent result = source;
    result.type = SamplerRenderEventType::noteOn;
    result.midiNote = 0;
    // V1 has no authored pedal-velocity enablement. Use the deterministic full trigger
    // velocity for layer selection; fixed-root route pitch is resolved by the voice pool.
    result.velocity = 1.0f;
    result.noteOffVelocity = 0.0f;
    result.articulationIndex = articulationIndex;
    result.performanceEvent = kind;
    result.sustainPedalDown = pedalDown;
    return result;
}

bool PerformanceLaneState::normalize(const SamplerRenderEvent& raw,
                                     const std::uint64_t activationGeneration,
                                     const CompiledPerformanceProgram& program,
                                     PerformanceActionScratch& scratch) noexcept
{
    auto event = raw;
    switch (raw.type)
    {
        case SamplerRenderEventType::noteOn:
        {
            const auto activation = program.activationByMidiNote[raw.midiNote];
            const auto isActivation = activation.articulationIndex < program.articulationCount;
            const auto articulationChanged = isActivation
                && selectedArticulationIndex != activation.articulationIndex;
            const auto emitsRoundRobinReset = articulationChanged
                && hasRoundRobinResetRule(program, RoundRobinResetEvent::articulationChange);
            const auto actionCount = (isActivation && activation.consume ? 0u : 1u)
                + (emitsRoundRobinReset ? 1u : 0u);
            if (scratch.size() + actionCount > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            if (isActivation)
            {
                selectedArticulationIndex = activation.articulationIndex;
                selectedArticulationStableId = activation.articulationIndex < program.articulationStableIds.size()
                    ? program.articulationStableIds[activation.articulationIndex] : 0;
            }
            recordNoteOn(event, activationGeneration, isActivation && activation.consume);
            ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOn)];
            if (emitsRoundRobinReset)
            {
                event.type = SamplerRenderEventType::roundRobinReset;
                event.roundRobinResetEvent = RoundRobinResetEvent::articulationChange;
                scratch.push(event);
                event = raw;
            }
            if (isActivation && activation.consume) return true;
            event.articulationIndex = selectedArticulationIndex;
            event.performanceEvent = PerformanceEventKind::noteOn;
            event.sustainPedalDown = pedalIsDown;
            return scratch.push(event);
        }
        case SamplerRenderEventType::noteOff:
        {
            auto& held = heldNotes[heldIndex(event.midiChannel, event.midiNote)];
            const auto consumed = held.active && held.consumed;
            const auto emitsPhysicalTrigger = held.active && !consumed
                && hasTriggerRoute(program, PerformanceEventKind::noteOff, held.articulationAtAttack, pedalIsDown);
            const auto emitsEffectiveRelease = held.active && !consumed && !pedalIsDown && !held.releaseEmitted;
            const auto emitsReleaseTrigger = emitsEffectiveRelease
                && hasTriggerRoute(program, PerformanceEventKind::release, held.articulationAtAttack, false);
            const auto actionCount = consumed ? 0u : (1u + (emitsPhysicalTrigger ? 1u : 0u)
                + (emitsReleaseTrigger ? 1u : 0u));
            if (scratch.size() + actionCount > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            recordNoteOff(event);
            ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOff)];
            if (consumed)
            {
                held = {};
                return true;
            }
            if (!held.active)
            {
                event.performanceEvent = PerformanceEventKind::noteOff;
                event.sustainPedalDown = pedalIsDown;
                return scratch.push(event);
            }
            event.articulationIndex = held.articulationAtAttack;
            event.performanceEvent = PerformanceEventKind::noteOff;
            event.sustainPedalDown = pedalIsDown;
            if (emitsPhysicalTrigger)
                scratch.push(makeTriggerEvent(event, held, PerformanceEventKind::noteOff, pedalIsDown));
            scratch.push(event);
            if (emitsEffectiveRelease)
            {
                if (emitsReleaseTrigger)
                    scratch.push(makeTriggerEvent(event, held, PerformanceEventKind::release, false));
                held.releaseEmitted = true;
                held.pedalDeferred = false;
                held.active = false;
                ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::release)];
            }
            return true;
        }
        case SamplerRenderEventType::sustainPedal:
        {
            const auto down = event.velocity >= (64.0f / 127.0f);
            if (down == pedalIsDown) return true;
            const auto pedalEvent = down ? PerformanceEventKind::pedalDown : PerformanceEventKind::pedalUp;
            const auto emitsPedalTrigger = hasTriggerRoute(program, pedalEvent, selectedArticulationIndex, down);
            std::size_t releaseCount = 0;
            if (!down)
                for (const auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted
                        && hasTriggerRoute(program, PerformanceEventKind::release,
                                           held.articulationAtAttack, false))
                        ++releaseCount;
            if (scratch.size() + 1 + releaseCount + (emitsPedalTrigger ? 1u : 0u)
                > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            setPedal(down);
            event.type = down ? SamplerRenderEventType::pedalDown : SamplerRenderEventType::pedalUp;
            event.performanceEvent = pedalEvent;
            event.sustainPedalDown = down;
            ++semanticEventCounts[static_cast<std::size_t>(down ? PerformanceEventKind::pedalDown
                                                                  : PerformanceEventKind::pedalUp)];
            scratch.push(event);
            if (!down)
                for (auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted)
                    {
                        if (hasTriggerRoute(program, PerformanceEventKind::release,
                                            held.articulationAtAttack, false))
                            scratch.push(makeTriggerEvent(event, held, PerformanceEventKind::release, false));
                        held.releaseEmitted = true;
                        held.pedalDeferred = false;
                        held.active = false;
                        ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::release)];
                    }
            if (emitsPedalTrigger)
                scratch.push(makePedalTriggerEvent(event, selectedArticulationIndex, pedalEvent, down));
            return true;
        }
        case SamplerRenderEventType::pedalDown:
        case SamplerRenderEventType::pedalUp:
        {
            const auto down = raw.type == SamplerRenderEventType::pedalDown;
            if (down == pedalIsDown) return true;
            const auto pedalEvent = down ? PerformanceEventKind::pedalDown : PerformanceEventKind::pedalUp;
            const auto emitsPedalTrigger = hasTriggerRoute(program, pedalEvent, selectedArticulationIndex, down);
            std::size_t releaseCount = 0;
            if (!down)
                for (const auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted
                        && hasTriggerRoute(program, PerformanceEventKind::release,
                                           held.articulationAtAttack, false))
                        ++releaseCount;
            if (scratch.size() + 1 + releaseCount + (emitsPedalTrigger ? 1u : 0u)
                > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            setPedal(down);
            event.performanceEvent = pedalEvent;
            event.sustainPedalDown = down;
            ++semanticEventCounts[static_cast<std::size_t>(down ? PerformanceEventKind::pedalDown
                                                                  : PerformanceEventKind::pedalUp)];
            scratch.push(event);
            if (!down)
                for (auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted)
                    {
                        if (hasTriggerRoute(program, PerformanceEventKind::release,
                                            held.articulationAtAttack, false))
                            scratch.push(makeTriggerEvent(event, held, PerformanceEventKind::release, false));
                        held.releaseEmitted = true;
                        held.pedalDeferred = false;
                        held.active = false;
                        ++semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::release)];
                    }
            if (emitsPedalTrigger)
                scratch.push(makePedalTriggerEvent(event, selectedArticulationIndex, pedalEvent, down));
            return true;
        }
        case SamplerRenderEventType::allNotesOff:
            if (scratch.size() + 1 > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            heldNotes = {};
            return scratch.push(event);
        case SamplerRenderEventType::reset:
            if (scratch.size() + 1 > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
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
