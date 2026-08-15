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
                     const bool pedalDown,
                     const int controllerNumber = -1,
                     const int controllerValue = 0) noexcept
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
        if (event == PerformanceEventKind::controllerChange
            && (controllerNumber < 0
                || route.triggerControllerNumber != controllerNumber
                || controllerValue < route.triggerControllerMinimum
                || controllerValue > route.triggerControllerMaximum))
            continue;
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
    controllerStateInitialized = false;
    continuousDamperEnabled = false;
    sustainControllerNumber = legacySustainControllerNumber;
    sustainThreshold = legacySustainThreshold;
    controllerValues = {};
    actionOverflowCount = 0;
    semanticEventCounts = {};
}

void PerformanceLaneState::migrateProgram(const CompiledPerformanceProgram& program,
                                          const std::uint64_t activationGeneration,
                                          const bool useContinuousDamper,
                                          const int configuredSustainController,
                                          const double configuredSustainThreshold) noexcept
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
    continuousDamperEnabled = useContinuousDamper;
    sustainControllerNumber = std::clamp(configuredSustainController, 0, 127);
    sustainThreshold = std::clamp(configuredSustainThreshold, 0.0, 127.0);
    if (!continuousDamperEnabled || !controllerStateInitialized)
    {
        controllerValues.fill(0);
        for (std::size_t controller = 0; controller < controllerValues.size(); ++controller)
            if (program.hasControllerDefault[controller])
                controllerValues[controller] = program.controllerDefaults[controller];
        controllerStateInitialized = true;
    }
    pedalIsDown = static_cast<double>(controllerValues[static_cast<std::size_t>(sustainControllerNumber)])
        >= sustainThreshold;
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
            if (continuousDamperEnabled)
            {
                event.type = SamplerRenderEventType::controllerChange;
                event.controllerNumber = halfPedalReleaseControllerNumber;
                event.controllerValue = raw.controllerNumber == halfPedalReleaseControllerNumber
                    ? raw.controllerValue
                    : toMidiVelocity(raw.velocity);
                return normalize(event, activationGeneration, program, scratch);
            }
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
        case SamplerRenderEventType::controllerChange:
        {
            if (raw.controllerNumber > 127 || raw.controllerValue > 127)
                return false;
            const auto controllerIndex = static_cast<std::size_t>(raw.controllerNumber);
            const auto oldValue = controllerValues[controllerIndex];
            const auto targetsSustain = continuousDamperEnabled
                && raw.controllerNumber == sustainControllerNumber;
            const auto wasDown = targetsSustain
                && static_cast<double>(oldValue) >= sustainThreshold;
            const auto isDown = targetsSustain
                && static_cast<double>(raw.controllerValue) >= sustainThreshold;
            const auto pedalEdge = targetsSustain && wasDown != isDown;
            const auto emitsControllerTrigger = hasTriggerRoute(
                program,
                PerformanceEventKind::controllerChange,
                selectedArticulationIndex,
                pedalEdge ? isDown : pedalIsDown,
                raw.controllerNumber,
                raw.controllerValue);
            const auto pedalEvent = isDown ? PerformanceEventKind::pedalDown
                                           : PerformanceEventKind::pedalUp;
            const auto emitsPedalTrigger = pedalEdge
                && hasTriggerRoute(program, pedalEvent, selectedArticulationIndex, isDown);
            std::size_t releaseCount = 0;
            if (pedalEdge && !isDown)
                for (const auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted
                        && hasTriggerRoute(program, PerformanceEventKind::release,
                                           held.articulationAtAttack, false))
                        ++releaseCount;
            const auto actionCount = 1u + (emitsControllerTrigger ? 1u : 0u)
                + (pedalEdge ? 1u : 0u) + releaseCount
                + (emitsPedalTrigger ? 1u : 0u);
            if (scratch.size() + actionCount
                > PerformanceActionScratch::capacity)
            {
                ++actionOverflowCount;
                return false;
            }
            controllerValues[controllerIndex] = raw.controllerValue;
            scratch.push(raw);
            ++semanticEventCounts[static_cast<std::size_t>(
                PerformanceEventKind::controllerChange)];
            if (emitsControllerTrigger)
            {
                event.type = SamplerRenderEventType::noteOn;
                event.midiNote = 0;
                event.velocity = 1.0f;
                event.articulationIndex = selectedArticulationIndex;
                event.performanceEvent = PerformanceEventKind::controllerChange;
                event.sustainPedalDown = pedalEdge ? isDown : pedalIsDown;
                scratch.push(event);
            }
            if (!pedalEdge)
                return true;

            setPedal(isDown);
            auto transition = raw;
            transition.type = isDown ? SamplerRenderEventType::pedalDown
                                     : SamplerRenderEventType::pedalUp;
            transition.performanceEvent = pedalEvent;
            transition.sustainPedalDown = isDown;
            ++semanticEventCounts[static_cast<std::size_t>(pedalEvent)];
            if (!isDown)
                for (auto& held : heldNotes)
                    if (held.active && !held.consumed && !held.physicalKeyDown
                        && held.pedalDeferred && !held.releaseEmitted)
                    {
                        if (hasTriggerRoute(program, PerformanceEventKind::release,
                                            held.articulationAtAttack, false))
                            scratch.push(makeTriggerEvent(transition, held,
                                                          PerformanceEventKind::release, false));
                        held.releaseEmitted = true;
                        held.pedalDeferred = false;
                        held.active = false;
                        ++semanticEventCounts[static_cast<std::size_t>(
                            PerformanceEventKind::release)];
                    }
            scratch.push(transition);
            if (emitsPedalTrigger)
                scratch.push(makePedalTriggerEvent(transition, selectedArticulationIndex,
                                                   pedalEvent, isDown));
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
            controllerValues.fill(0);
            for (std::size_t controller = 0; controller < controllerValues.size(); ++controller)
                if (program.hasControllerDefault[controller])
                    controllerValues[controller] = program.controllerDefaults[controller];
            controllerStateInitialized = true;
            pedalIsDown = static_cast<double>(controllerValues[static_cast<std::size_t>(
                sustainControllerNumber)]) >= sustainThreshold;
            return scratch.push(event);
    }
    return false;
}

PerformanceLaneStateSnapshot PerformanceLaneState::getSnapshot() const noexcept
{
    PerformanceLaneStateSnapshot result;
    result.selectedArticulationIndex = selectedArticulationIndex;
    result.pedalDown = pedalIsDown;
    result.controllerValues = controllerValues;
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
