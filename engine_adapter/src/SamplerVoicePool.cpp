#include "drs/engine/SamplerVoicePool.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr std::size_t crossfadeRouteLimit = 2;

bool routeMatches(const SamplerRenderRoute& route,
                  int midiNote,
                  int velocity,
                  int roundRobinPosition = 0) noexcept
{
    const auto rangeMatches = midiNote >= route.keyLow && midiNote <= route.keyHigh
        && velocity >= route.velocityLow && velocity <= route.velocityHigh;
    if (!rangeMatches)
        return false;

    if (route.roundRobinLength <= 0 || route.roundRobinPosition <= 0 || roundRobinPosition <= 0)
        return true;

    return route.roundRobinPosition == roundRobinPosition;
}

int resolveRoundRobinPosition(const std::vector<SamplerRenderRoute>& routes,
                              int midiNote,
                              int velocity,
                              std::uint64_t voiceId) noexcept
{
    int roundRobinLength = 0;
    for (const auto& route : routes)
    {
        if (midiNote < route.keyLow || midiNote > route.keyHigh
            || velocity < route.velocityLow || velocity > route.velocityHigh)
        {
            continue;
        }

        roundRobinLength = std::max(roundRobinLength, route.roundRobinLength);
    }

    if (roundRobinLength <= 0)
        return 0;

    return static_cast<int>(((voiceId - 1) % static_cast<std::uint64_t>(roundRobinLength)) + 1);
}

bool routeHasCrossfade(const SamplerRenderRoute& route) noexcept
{
    return hasAnyVelocityCrossfadeValue(route.velocityCrossfade);
}

bool routeHasCrossfadeRuntime(const SamplerRenderRoute& route) noexcept
{
    return hasAnyVelocityCrossfadeRuntimeValue(route.velocityCrossfadeRuntime);
}

double computeRouteCrossfadeGain(const SamplerRenderRoute& route, int velocity) noexcept
{
    return computeFirstPassVelocityCrossfadeGain(
        { route.velocityLow, route.velocityHigh, route.velocityCrossfade },
        velocity);
}

bool areCrossfadeNeighbors(const SamplerRenderRoute& left,
                           const SamplerRenderRoute& right) noexcept
{
    return left.velocityCrossfadeRuntime.fadeInNeighborZoneId == right.zoneId
        || left.velocityCrossfadeRuntime.fadeOutNeighborZoneId == right.zoneId
        || right.velocityCrossfadeRuntime.fadeInNeighborZoneId == left.zoneId
        || right.velocityCrossfadeRuntime.fadeOutNeighborZoneId == left.zoneId;
}
} // namespace

bool SamplerEventBlock::push(SamplerRenderEvent event) noexcept
{
    if (eventCount >= events.size())
    {
        ++droppedEvents;
        return false;
    }

    auto insertionIndex = eventCount;
    while (insertionIndex > 0 && events[insertionIndex - 1].sampleOffset > event.sampleOffset)
    {
        events[insertionIndex] = events[insertionIndex - 1];
        --insertionIndex;
    }
    events[insertionIndex] = event;
    ++eventCount;
    return true;
}

void SamplerEventBlock::clear() noexcept
{
    eventCount = 0;
    droppedEvents = 0;
}

bool SamplerVoicePool::prepare(const SamplerRenderModel& model,
                               double outputSampleRate,
                               std::uint64_t activationGeneration) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0
        || model.getRoutes().empty() || model.getSamples().empty())
    {
        return false;
    }

    resetVoices();
    nextVoiceId = 1;
    nextTriggerId = 1;
    nextGeneratedActivation = 1;
    return activateModel(model, outputSampleRate, activationGeneration);
}

bool SamplerVoicePool::activateModel(const SamplerRenderModel& model,
                                     double outputSampleRate,
                                     std::uint64_t activationGeneration) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0
        || model.getRoutes().empty() || model.getSamples().empty())
    {
        return false;
    }

    if (activationGeneration == 0)
    {
        activationGeneration = nextGeneratedActivation++;
        if (nextGeneratedActivation == 0)
            nextGeneratedActivation = 1;
    }
    else if (activationGeneration >= nextGeneratedActivation)
    {
        nextGeneratedActivation = activationGeneration + 1;
        if (nextGeneratedActivation == 0)
            nextGeneratedActivation = 1;
    }

    renderModel = &model;
    sampleRate = outputSampleRate;
    activeGeneration = activationGeneration;
    return true;
}

void SamplerVoicePool::clearRenderModel() noexcept
{
    resetVoices();
    renderModel = nullptr;
    sampleRate = 0.0;
    nextVoiceId = 1;
    nextTriggerId = 1;
    activeGeneration = 0;
    nextGeneratedActivation = 1;
}

SamplerVoicePoolRenderResult SamplerVoicePool::renderBlock(SamplerAudioBufferView output,
                                                           SamplerRenderEventView events,
                                                           SamplerRenderControlValues controls) noexcept
{
    SamplerVoicePoolRenderResult result;
    if (renderModel == nullptr || !output.isValid() || !events.isValid() || events.size > SamplerEventBlock::capacity)
        return result;

    std::uint32_t previousOffset = 0;
    for (std::size_t index = 0; index < events.size; ++index)
    {
        if (events[index].sampleOffset >= output.frameCount
            || (index > 0 && events[index].sampleOffset < previousOffset))
        {
            return result;
        }
        previousOffset = events[index].sampleOffset;
    }

    result.accepted = true;
    std::uint32_t renderedThrough = 0;
    for (std::size_t index = 0; index < events.size; ++index)
    {
        const auto eventOffset = events[index].sampleOffset;
        renderRange(output, renderedThrough, eventOffset - renderedThrough, result);
        renderedThrough = eventOffset;
        applyEvent(events[index], result, controls);
        ++result.render.consumedEventCount;
    }
    renderRange(output, renderedThrough, output.frameCount - renderedThrough, result);
    result.render.renderedFrameCount = output.frameCount;
    updateCounts(result);
    return result;
}

void SamplerVoicePool::resetVoices() noexcept
{
    for (auto& slot : slots)
    {
        slot.voice.reset();
        slot.state = SamplerVoiceSlotState::free;
        slot.sustainDeferred = false;
    }
    sustainPedalDown = false;
}

std::size_t SamplerVoicePool::activeVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [](const Slot& slot)
    {
        return slot.state == SamplerVoiceSlotState::active;
    }));
}

std::size_t SamplerVoicePool::releasingVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [](const Slot& slot)
    {
        return slot.state == SamplerVoiceSlotState::releasing;
    }));
}

std::size_t SamplerVoicePool::finishedVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [](const Slot& slot)
    {
        return slot.state == SamplerVoiceSlotState::finished;
    }));
}

std::size_t SamplerVoicePool::voiceCountUsingModel(const SamplerRenderModel* model) const noexcept
{
    if (model == nullptr)
        return 0;
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [model](const Slot& slot)
    {
        return (slot.state == SamplerVoiceSlotState::active
                || slot.state == SamplerVoiceSlotState::releasing)
            && slot.voice.getRenderModel() == model;
    }));
}

std::size_t SamplerVoicePool::voiceCountUsingGeneration(
    std::uint64_t activationGeneration) const noexcept
{
    if (activationGeneration == 0)
        return 0;
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(),
        [activationGeneration](const Slot& slot)
        {
            return (slot.state == SamplerVoiceSlotState::active
                    || slot.state == SamplerVoiceSlotState::releasing)
                && slot.voice.getActivationGeneration() == activationGeneration;
        }));
}

std::size_t SamplerVoicePool::retiredGenerationVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [&](const Slot& slot)
    {
        return (slot.state == SamplerVoiceSlotState::active
                || slot.state == SamplerVoiceSlotState::releasing)
            && slot.voice.getActivationGeneration() != activeGeneration;
    }));
}

std::size_t SamplerVoicePool::sustainDeferredVoiceCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(slots.begin(), slots.end(), [](const Slot& slot)
    {
        return slot.state == SamplerVoiceSlotState::active && slot.sustainDeferred;
    }));
}

SamplerVoiceSlotSnapshot SamplerVoicePool::getSlotSnapshot(std::size_t index) const noexcept
{
    if (index >= slots.size())
        return {};
    const auto& slot = slots[index];
    return { slot.state,
             slot.voice.getVoiceId(),
             slot.voice.getActivationGeneration(),
             slot.voice.getRenderModel() != nullptr
                 ? slot.voice.getRenderModel()->getRevision() : 0,
             slot.voice.getSourceMidiNote(),
             slot.voice.getEffectiveMidiNote(),
             slot.voice.getIncrementFrames(),
             slot.voice.getBaseGain(),
             slot.voice.isLoopActive(),
             slot.sustainDeferred };
}

void SamplerVoicePool::renderRange(SamplerAudioBufferView output,
                                   std::uint32_t startFrame,
                                   std::uint32_t frameCount,
                                   SamplerVoicePoolRenderResult& result) noexcept
{
    if (frameCount == 0)
        return;

    for (auto& slot : slots)
    {
        if (slot.state != SamplerVoiceSlotState::active
            && slot.state != SamplerVoiceSlotState::releasing)
        {
            continue;
        }

        const auto voiceResult = slot.voice.render(output, startFrame, frameCount);
        if (voiceResult.voiceFinished)
        {
            slot.state = SamplerVoiceSlotState::finished;
            slot.sustainDeferred = false;
            ++result.render.completedVoiceCount;
        }
    }
}

void SamplerVoicePool::applyEvent(const SamplerRenderEvent& event,
                                  SamplerVoicePoolRenderResult& result,
                                  const SamplerRenderControlValues& controls) noexcept
{
    switch (event.type)
    {
        case SamplerRenderEventType::noteOn:
        {
            if (event.midiNote > 127 || !std::isfinite(event.velocity)
                || event.velocity <= 0.0f || event.velocity > 1.0f)
            {
                ++result.render.droppedEventCount;
                return;
            }

            const auto sourceMidiNote = static_cast<int>(event.midiNote);
            const auto midiNoteOffset = controls.overrideMidiNoteOffset
                ? controls.midiNoteOffset : renderModel->getMidiNoteOffset();
            const auto effectiveMidiNote = std::clamp(sourceMidiNote + midiNoteOffset, 0, 127);
            const auto eventVelocity = std::clamp(
                static_cast<int>(std::lround(event.velocity * 127.0f)), 1, 127);
            const auto fixedVelocity = controls.overrideFixedVelocity
                ? controls.fixedVelocity : renderModel->getFixedVelocity();
            const auto effectiveVelocity = fixedVelocity > 0
                ? fixedVelocity
                : eventVelocity;
            // Route the physical gesture first. Published pitch/velocity modulation shapes the
            // started voices and must not make otherwise playable authored zones disappear.
            // The effective-velocity fallback preserves legacy fixed-layer presets when the
            // physical velocity has no route in the selected articulation.
            const auto& routes = renderModel->getRoutes();
            const auto physicalRoundRobinPosition =
                resolveRoundRobinPosition(routes, sourceMidiNote, eventVelocity, nextTriggerId);
            const auto hasPhysicalVelocityRoute = std::any_of(routes.begin(), routes.end(), [&](const auto& route)
            {
                return routeMatches(route, sourceMidiNote, eventVelocity, physicalRoundRobinPosition);
            });
            const auto routingVelocity = hasPhysicalVelocityRoute ? eventVelocity : effectiveVelocity;
            const auto routingRoundRobinPosition = hasPhysicalVelocityRoute
                ? physicalRoundRobinPosition
                : resolveRoundRobinPosition(routes, sourceMidiNote, effectiveVelocity, nextTriggerId);
            const auto hasMatchingRoute = hasPhysicalVelocityRoute
                || (effectiveVelocity != eventVelocity
                    && std::any_of(routes.begin(), routes.end(), [&](const auto& route)
                    {
                        return routeMatches(route, sourceMidiNote, effectiveVelocity, routingRoundRobinPosition);
                    }));
            if (!hasMatchingRoute)
            {
                ++result.render.droppedEventCount;
                return;
            }

            struct CrossfadeCandidate
            {
                std::size_t routeIndex = 0;
                double gainMultiplier = 1.0;
            };

            std::array<CrossfadeCandidate, crossfadeRouteLimit> crossfadeCandidates {};
            std::size_t positiveCrossfadeCount = 0;
            bool hasCrossfadeMatch = false;
            bool hasNonCrossfadeMatch = false;
            bool crossfadeFallback = false;

            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex)
            {
                if (!routeMatches(routes[routeIndex], sourceMidiNote, routingVelocity, routingRoundRobinPosition))
                    continue;

                if (!routeHasCrossfade(routes[routeIndex]))
                {
                    hasNonCrossfadeMatch = true;
                    continue;
                }

                hasCrossfadeMatch = true;
                if (!routeHasCrossfadeRuntime(routes[routeIndex]))
                {
                    crossfadeFallback = true;
                    continue;
                }

                const auto gainMultiplier = computeRouteCrossfadeGain(routes[routeIndex], routingVelocity);
                if (!std::isfinite(gainMultiplier) || gainMultiplier <= 0.0)
                    continue;

                if (positiveCrossfadeCount < crossfadeCandidates.size())
                    crossfadeCandidates[positiveCrossfadeCount] = { routeIndex, gainMultiplier };
                ++positiveCrossfadeCount;
            }

            auto useLegacyRouting = !hasCrossfadeMatch;
            if (hasCrossfadeMatch)
            {
                if (crossfadeFallback
                    || positiveCrossfadeCount == 0
                    || positiveCrossfadeCount > crossfadeRouteLimit)
                {
                    useLegacyRouting = true;
                }
                else if (positiveCrossfadeCount == crossfadeRouteLimit)
                {
                    const auto& firstRoute = routes[crossfadeCandidates[0].routeIndex];
                    const auto& secondRoute = routes[crossfadeCandidates[1].routeIndex];
                    useLegacyRouting = !areCrossfadeNeighbors(firstRoute, secondRoute);
                }
                else
                {
                    useLegacyRouting = false;
                }

                if (useLegacyRouting)
                    ++result.render.crossfadeFallbackCount;
            }

            const auto startRoute = [&](std::size_t routeIndex,
                                        double routeGainMultiplier,
                                        bool countAsCrossfade) noexcept
            {
                bool stolen = false;
                bool generationStolen = false;
                bool releasingStolen = false;
                const auto slotIndex = acquireSlot(stolen, generationStolen, releasingStolen);
                auto& slot = slots[slotIndex];
                SamplerVoiceStartRequest request;
                request.voiceId = nextVoiceId;
                request.activationGeneration = activeGeneration;
                request.routeIndex = routeIndex;
                request.sourceMidiNote = sourceMidiNote;
                request.effectiveMidiNote = effectiveMidiNote;
                request.effectiveVelocity = effectiveVelocity;
                request.routeGainMultiplier = routeGainMultiplier;
                request.outputSampleRate = sampleRate;
                if (!slot.voice.start(*renderModel, request))
                {
                    slot.state = SamplerVoiceSlotState::free;
                    slot.sustainDeferred = false;
                    ++result.render.droppedEventCount;
                    return false;
                }

                slot.state = SamplerVoiceSlotState::active;
                slot.sustainDeferred = false;
                ++nextVoiceId;
                if (nextVoiceId == 0)
                    nextVoiceId = 1;
                ++result.render.startedVoiceCount;
                if (countAsCrossfade)
                    ++result.render.crossfadeStartedVoiceCount;
                if (stolen)
                    ++result.render.stolenVoiceCount;
                if (generationStolen)
                    ++result.render.generationStealCount;
                if (releasingStolen)
                    ++result.render.releasingVoiceStealCount;
                return true;
            };

            if (useLegacyRouting)
            {
                for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex)
                {
                    if (!routeMatches(routes[routeIndex], sourceMidiNote, routingVelocity, routingRoundRobinPosition))
                        continue;

                    startRoute(routeIndex, 1.0, false);
                }
                ++nextTriggerId;
                if (nextTriggerId == 0)
                    nextTriggerId = 1;
                return;
            }

            for (std::size_t routeIndex = 0; routeIndex < routes.size(); ++routeIndex)
            {
                if (!routeMatches(routes[routeIndex], sourceMidiNote, routingVelocity, routingRoundRobinPosition)
                    || routeHasCrossfade(routes[routeIndex]))
                {
                    continue;
                }

                startRoute(routeIndex, 1.0, false);
            }

            std::size_t crossfadeStartedForEvent = 0;
            for (std::size_t candidateIndex = 0;
                 candidateIndex < std::min(positiveCrossfadeCount, crossfadeCandidates.size());
                 ++candidateIndex)
            {
                if (startRoute(crossfadeCandidates[candidateIndex].routeIndex,
                               crossfadeCandidates[candidateIndex].gainMultiplier,
                               true))
                {
                    ++crossfadeStartedForEvent;
                }
            }

            if (crossfadeStartedForEvent >= 2)
                ++result.render.crossfadeOverlapHitCount;
            ++nextTriggerId;
            if (nextTriggerId == 0)
                nextTriggerId = 1;
            return;
        }

        case SamplerRenderEventType::noteOff:
            if (event.midiNote > 127)
            {
                ++result.render.droppedEventCount;
                return;
            }
            for (auto& slot : slots)
            {
                if (slot.state == SamplerVoiceSlotState::active
                    && slot.voice.getSourceMidiNote() == static_cast<int>(event.midiNote))
                {
                    if (slot.voice.ignoresNoteOff())
                    {
                        slot.sustainDeferred = false;
                        continue;
                    }
                    if (sustainPedalDown)
                    {
                        slot.sustainDeferred = true;
                    }
                    else if (slot.voice.beginRelease())
                    {
                        slot.state = SamplerVoiceSlotState::releasing;
                        slot.sustainDeferred = false;
                        ++result.render.releasedVoiceCount;
                    }
                }
            }
            return;

        case SamplerRenderEventType::sustainPedal:
        {
            const auto pressed = event.velocity >= 0.5f;
            if (pressed == sustainPedalDown)
                return;
            sustainPedalDown = pressed;
            if (sustainPedalDown)
                return;
            for (auto& slot : slots)
            {
                if (slot.state == SamplerVoiceSlotState::active
                    && slot.sustainDeferred
                    && slot.voice.beginRelease())
                {
                    slot.state = SamplerVoiceSlotState::releasing;
                    slot.sustainDeferred = false;
                    ++result.render.releasedVoiceCount;
                }
            }
            return;
        }

        case SamplerRenderEventType::allNotesOff:
            for (auto& slot : slots)
            {
                if (slot.state == SamplerVoiceSlotState::active)
                {
                    if (slot.voice.beginRelease())
                    {
                        slot.state = SamplerVoiceSlotState::releasing;
                        slot.sustainDeferred = false;
                        ++result.render.releasedVoiceCount;
                    }
                }
            }
            return;

        case SamplerRenderEventType::reset:
            for (auto& slot : slots)
            {
                if (slot.state == SamplerVoiceSlotState::active
                    || slot.state == SamplerVoiceSlotState::releasing)
                {
                    ++result.resetVoiceCount;
                }
                slot.voice.reset();
                slot.state = SamplerVoiceSlotState::free;
                slot.sustainDeferred = false;
            }
            sustainPedalDown = false;
            return;
    }

    ++result.render.droppedEventCount;
}

std::size_t SamplerVoicePool::acquireSlot(bool& stolen,
                                          bool& generationStolen,
                                          bool& releasingStolen) noexcept
{
    stolen = false;
    generationStolen = false;
    releasingStolen = false;
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        if (slots[index].state == SamplerVoiceSlotState::free
            || slots[index].state == SamplerVoiceSlotState::finished)
        {
            slots[index].voice.reset();
            slots[index].sustainDeferred = false;
            return index;
        }
    }

    auto selected = slots.size();
    auto selectedRank = std::numeric_limits<int>::max();
    auto selectedGeneration = std::numeric_limits<std::uint64_t>::max();
    auto selectedVoiceId = std::numeric_limits<std::uint64_t>::max();
    const auto performancePolicy = renderModel != nullptr
        && renderModel->getLane() == PlaybackActivationLane::performance;
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        const auto& slot = slots[index];
        if (slot.state != SamplerVoiceSlotState::active
            && slot.state != SamplerVoiceSlotState::releasing)
            continue;
        const auto retiredGeneration
            = slot.voice.getActivationGeneration() != activeGeneration;
        const auto rank = performancePolicy
            ? (slot.state == SamplerVoiceSlotState::active
                   ? (retiredGeneration ? 0 : 1)
                   : (retiredGeneration ? 2 : 3))
            : (slot.state == SamplerVoiceSlotState::releasing ? 0 : 1);
        const auto generation = slot.voice.getActivationGeneration();
        const auto voiceId = slot.voice.getVoiceId();
        if (rank < selectedRank
            || (rank == selectedRank && generation < selectedGeneration)
            || (rank == selectedRank && generation == selectedGeneration
                && voiceId < selectedVoiceId))
        {
            selected = index;
            selectedRank = rank;
            selectedGeneration = generation;
            selectedVoiceId = voiceId;
        }
    }

    if (selected == slots.size())
        selected = 0;
    generationStolen = slots[selected].voice.getActivationGeneration() != activeGeneration;
    releasingStolen = slots[selected].state == SamplerVoiceSlotState::releasing;
    slots[selected].voice.reset();
    slots[selected].sustainDeferred = false;
    stolen = true;
    return selected;
}

void SamplerVoicePool::updateCounts(SamplerVoicePoolRenderResult& result) const noexcept
{
    result.activeVoiceCount = static_cast<std::uint32_t>(activeVoiceCount());
    result.releasingVoiceCount = static_cast<std::uint32_t>(releasingVoiceCount());
    result.finishedVoiceCount = static_cast<std::uint32_t>(finishedVoiceCount());
    result.activeGenerationVoiceCount = static_cast<std::uint32_t>(
        voiceCountUsingGeneration(activeGeneration));
    result.retiredGenerationVoiceCount = static_cast<std::uint32_t>(
        retiredGenerationVoiceCount());
    result.sustainDeferredVoiceCount = static_cast<std::uint32_t>(
        sustainDeferredVoiceCount());
}
} // namespace drs::engine
