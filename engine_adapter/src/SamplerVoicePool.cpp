#include "drs/engine/SamplerVoicePool.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr std::size_t crossfadeRouteLimit = 2;

bool hasExplicitRoundRobin(const SamplerRenderRoute& route) noexcept
{
    return route.roundRobin.has_value()
        && route.roundRobin->slotCount > 0
        && route.roundRobin->slotIndex > 0;
}

int resolveRoundRobinSlotCount(const SamplerRenderRoute& route) noexcept
{
    if (hasExplicitRoundRobin(route))
        return route.roundRobin->slotCount;

    return route.roundRobinLength;
}

int resolveRoundRobinSlotIndex(const SamplerRenderRoute& route) noexcept
{
    if (hasExplicitRoundRobin(route))
        return route.roundRobin->slotIndex;

    return route.roundRobinPosition;
}

bool routeUsesRoundRobin(const SamplerRenderRoute& route) noexcept
{
    return resolveRoundRobinSlotCount(route) > 0
        && resolveRoundRobinSlotIndex(route) > 0;
}

std::string_view resolveRoundRobinPoolId(const SamplerRenderRoute& route) noexcept
{
    if (hasExplicitRoundRobin(route) && !route.roundRobin->poolId.empty())
        return route.roundRobin->poolId;

    return {};
}

bool usesLegacyRoundRobinKey(const SamplerRenderRoute& route) noexcept
{
    return routeUsesRoundRobin(route) && resolveRoundRobinPoolId(route).empty();
}

RoundRobinMode resolveRoundRobinMode(const SamplerRenderRoute& route) noexcept
{
    return hasExplicitRoundRobin(route)
        ? route.roundRobin->mode
        : RoundRobinMode::sequential;
}

bool sameRoundRobinPool(std::string_view leftPoolId,
                        int leftSlotCount,
                        bool leftUsesLegacyScalarKey,
                        RoundRobinMode leftMode,
                        std::string_view rightPoolId,
                        int rightSlotCount,
                        bool rightUsesLegacyScalarKey,
                        RoundRobinMode rightMode) noexcept
{
    return leftSlotCount == rightSlotCount
        && leftUsesLegacyScalarKey == rightUsesLegacyScalarKey
        && leftMode == rightMode
        && (leftUsesLegacyScalarKey || leftPoolId == rightPoolId);
}

bool routeCouldRespondToTrigger(const SamplerRenderRoute& route,
                                int midiNote,
                                int physicalVelocity,
                                int effectiveVelocity) noexcept
{
    if (midiNote < route.keyLow || midiNote > route.keyHigh)
        return false;

    const auto physicalVelocityMatches = physicalVelocity >= route.velocityLow
        && physicalVelocity <= route.velocityHigh;
    const auto effectiveVelocityMatches = effectiveVelocity >= route.velocityLow
        && effectiveVelocity <= route.velocityHigh;
    return physicalVelocityMatches || effectiveVelocityMatches;
}

int advanceRoundRobinSlotIndex(int currentSlotIndex, int slotCount) noexcept
{
    if (slotCount <= 1)
        return 1;

    ++currentSlotIndex;
    if (currentSlotIndex > slotCount || currentSlotIndex <= 0)
        currentSlotIndex = 1;
    return currentSlotIndex;
}

std::uint64_t computeFnv1a64(std::string_view text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::uint64_t advanceRandomState(std::uint64_t state) noexcept
{
    state += 0x9e3779b97f4a7c15ull;
    state = (state ^ (state >> 30u)) * 0xbf58476d1ce4e5b9ull;
    state = (state ^ (state >> 27u)) * 0x94d049bb133111ebull;
    return state ^ (state >> 31u);
}

struct SelectedRoundRobinSlot
{
    std::string_view poolId;
    int slotCount = 0;
    bool usesLegacyScalarKey = false;
    RoundRobinMode mode = RoundRobinMode::sequential;
    int slotIndex = 0;
};

int eventPriorityAtSharedOffset(const SamplerRenderEvent& event) noexcept
{
    switch (event.type)
    {
        case SamplerRenderEventType::reset:
            return 0;
        case SamplerRenderEventType::allNotesOff:
            return 1;
        case SamplerRenderEventType::sustainPedal:
            return 2;
        case SamplerRenderEventType::noteOff:
            return 3;
        case SamplerRenderEventType::noteOn:
            return 4;
    }

    return std::numeric_limits<int>::max();
}

bool routeMatches(const SamplerRenderRoute& route,
                  int midiNote,
                  int velocity,
                  const SelectedRoundRobinSlot* roundRobinSelections,
                  std::size_t roundRobinSelectionCount) noexcept
{
    const auto rangeMatches = midiNote >= route.keyLow && midiNote <= route.keyHigh
        && velocity >= route.velocityLow && velocity <= route.velocityHigh;
    if (!rangeMatches)
        return false;

    if (!routeUsesRoundRobin(route))
        return true;

    const auto poolId = resolveRoundRobinPoolId(route);
    const auto slotCount = resolveRoundRobinSlotCount(route);
    const auto usesLegacyScalarKey = usesLegacyRoundRobinKey(route);
    for (std::size_t index = 0; index < roundRobinSelectionCount; ++index)
    {
        if (sameRoundRobinPool(roundRobinSelections[index].poolId,
                               roundRobinSelections[index].slotCount,
                               roundRobinSelections[index].usesLegacyScalarKey,
                               roundRobinSelections[index].mode,
                               poolId,
                               slotCount,
                               usesLegacyScalarKey,
                               resolveRoundRobinMode(route)))
        {
            return resolveRoundRobinSlotIndex(route) == roundRobinSelections[index].slotIndex;
        }
    }

    return false;
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
    while (insertionIndex > 0)
    {
        const auto& previous = events[insertionIndex - 1];
        if (previous.sampleOffset < event.sampleOffset)
            break;
        if (previous.sampleOffset == event.sampleOffset
            && eventPriorityAtSharedOffset(previous) <= eventPriorityAtSharedOffset(event))
        {
            break;
        }

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
    resetRoundRobinPools();
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
    rebuildRoundRobinPools(model);
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
    resetRoundRobinPools();
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
            std::array<SelectedRoundRobinSlot, roundRobinPoolCapacity> roundRobinSelections {};
            std::size_t roundRobinSelectionCount = 0;
            bool sawRoundRobinCandidate = false;
            for (const auto& route : renderModel->getRoutes())
            {
                if (!routeCouldRespondToTrigger(route, sourceMidiNote, eventVelocity, effectiveVelocity))
                    continue;

                if (!routeUsesRoundRobin(route))
                    continue;

                sawRoundRobinCandidate = true;
                const auto slotCount = resolveRoundRobinSlotCount(route);
                const auto poolId = resolveRoundRobinPoolId(route);
                const auto usesLegacyScalarKey = usesLegacyRoundRobinKey(route);
                const auto mode = resolveRoundRobinMode(route);
                if (roundRobinSelectionCount >= roundRobinSelections.size())
                {
                    ++result.render.roundRobinFallbackCount;
                    continue;
                }

                auto alreadySelected = false;
                for (std::size_t index = 0; index < roundRobinSelectionCount; ++index)
                {
                    if (sameRoundRobinPool(roundRobinSelections[index].poolId,
                                           roundRobinSelections[index].slotCount,
                                           roundRobinSelections[index].usesLegacyScalarKey,
                                           roundRobinSelections[index].mode,
                                           poolId,
                                           slotCount,
                                           usesLegacyScalarKey,
                                           mode))
                    {
                        alreadySelected = true;
                        break;
                    }
                }

                if (alreadySelected || roundRobinSelectionCount >= roundRobinSelections.size())
                    continue;

                int slotIndex = 1;
                const auto foundPool = peekRoundRobinSlot(poolId,
                                                          slotCount,
                                                          usesLegacyScalarKey,
                                                          mode,
                                                          slotIndex);
                roundRobinSelections[roundRobinSelectionCount].poolId = poolId;
                roundRobinSelections[roundRobinSelectionCount].slotCount = slotCount;
                roundRobinSelections[roundRobinSelectionCount].usesLegacyScalarKey = usesLegacyScalarKey;
                roundRobinSelections[roundRobinSelectionCount].mode = mode;
                roundRobinSelections[roundRobinSelectionCount].slotIndex = slotIndex;
                if (foundPool)
                {
                    ++result.render.roundRobinPoolHitCount;
                }
                else
                {
                    ++result.render.roundRobinPoolMissCount;
                    ++result.render.roundRobinFallbackCount;
                }
                ++roundRobinSelectionCount;
            }
            // Route the physical gesture first. Published pitch/velocity modulation shapes the
            // started voices and must not make otherwise playable authored zones disappear.
            // The effective-velocity fallback preserves legacy fixed-layer presets when the
            // physical velocity has no route in the selected articulation.
            const auto& routes = renderModel->getRoutes();
            const auto hasPhysicalVelocityRoute = std::any_of(routes.begin(), routes.end(), [&](const auto& route)
            {
                return routeMatches(route,
                                    sourceMidiNote,
                                    eventVelocity,
                                    roundRobinSelections.data(),
                                    roundRobinSelectionCount);
            });
            const auto routingVelocity = hasPhysicalVelocityRoute ? eventVelocity : effectiveVelocity;
            const auto hasMatchingRoute = hasPhysicalVelocityRoute
                || (effectiveVelocity != eventVelocity
                    && std::any_of(routes.begin(), routes.end(), [&](const auto& route)
                    {
                        return routeMatches(route,
                                            sourceMidiNote,
                                            effectiveVelocity,
                                            roundRobinSelections.data(),
                                            roundRobinSelectionCount);
                    }));
            if (!hasMatchingRoute)
            {
                if (sawRoundRobinCandidate)
                    ++result.render.roundRobinFallbackCount;
                ++result.render.droppedEventCount;
                return;
            }
            for (std::size_t index = 0; index < roundRobinSelectionCount; ++index)
            {
                if (!advanceRoundRobinSlot(roundRobinSelections[index].poolId,
                                           roundRobinSelections[index].slotCount,
                                           roundRobinSelections[index].usesLegacyScalarKey,
                                           roundRobinSelections[index].mode))
                {
                    ++result.render.roundRobinPoolMissCount;
                    ++result.render.roundRobinFallbackCount;
                }
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
                if (!routeMatches(routes[routeIndex],
                                  sourceMidiNote,
                                  routingVelocity,
                                  roundRobinSelections.data(),
                                  roundRobinSelectionCount))
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
                    if (!routeMatches(routes[routeIndex],
                                      sourceMidiNote,
                                      routingVelocity,
                                      roundRobinSelections.data(),
                                      roundRobinSelectionCount))
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
                if (!routeMatches(routes[routeIndex],
                                  sourceMidiNote,
                                  routingVelocity,
                                  roundRobinSelections.data(),
                                  roundRobinSelectionCount)
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

void SamplerVoicePool::resetRoundRobinPools() noexcept
{
    roundRobinPoolCount = 0;
    for (auto& pool : roundRobinPools)
        pool = {};
}

void SamplerVoicePool::rebuildRoundRobinPools(const SamplerRenderModel& model) noexcept
{
    resetRoundRobinPools();
    for (const auto& route : model.getRoutes())
    {
        if (!routeUsesRoundRobin(route) || roundRobinPoolCount >= roundRobinPools.size())
            continue;

        const auto poolId = resolveRoundRobinPoolId(route);
        const auto slotCount = resolveRoundRobinSlotCount(route);
        const auto usesLegacyScalarKey = usesLegacyRoundRobinKey(route);
        const auto mode = resolveRoundRobinMode(route);
        auto knownPool = false;
        for (std::size_t index = 0; index < roundRobinPoolCount; ++index)
        {
            if (sameRoundRobinPool(roundRobinPools[index].key.poolId,
                                   roundRobinPools[index].key.slotCount,
                                   roundRobinPools[index].key.usesLegacyScalarKey,
                                   roundRobinPools[index].key.mode,
                                   poolId,
                                   slotCount,
                                   usesLegacyScalarKey,
                                   mode))
            {
                knownPool = true;
                break;
            }
        }

        if (knownPool)
            continue;

        roundRobinPools[roundRobinPoolCount].key.poolId = poolId;
        roundRobinPools[roundRobinPoolCount].key.slotCount = slotCount;
        roundRobinPools[roundRobinPoolCount].key.usesLegacyScalarKey = usesLegacyScalarKey;
        roundRobinPools[roundRobinPoolCount].key.mode = mode;
        roundRobinPools[roundRobinPoolCount].randomState =
            computeFnv1a64(poolId) ^ static_cast<std::uint64_t>(slotCount);
        if (mode == RoundRobinMode::random)
        {
            roundRobinPools[roundRobinPoolCount].randomState =
                advanceRandomState(roundRobinPools[roundRobinPoolCount].randomState);
            roundRobinPools[roundRobinPoolCount].nextSlotIndex =
                static_cast<int>(roundRobinPools[roundRobinPoolCount].randomState
                                 % static_cast<std::uint64_t>(slotCount)) + 1;
        }
        else
        {
            roundRobinPools[roundRobinPoolCount].nextSlotIndex = 1;
        }
        ++roundRobinPoolCount;
    }
}

bool SamplerVoicePool::peekRoundRobinSlot(std::string_view poolId,
                                          int slotCount,
                                          bool usesLegacyScalarKey,
                                          RoundRobinMode mode,
                                          int& slotIndex) const noexcept
{
    if (slotCount <= 0)
    {
        slotIndex = 0;
        return false;
    }

    for (std::size_t index = 0; index < roundRobinPoolCount; ++index)
    {
        if (sameRoundRobinPool(roundRobinPools[index].key.poolId,
                               roundRobinPools[index].key.slotCount,
                               roundRobinPools[index].key.usesLegacyScalarKey,
                               roundRobinPools[index].key.mode,
                               poolId,
                               slotCount,
                               usesLegacyScalarKey,
                               mode))
        {
            slotIndex = roundRobinPools[index].nextSlotIndex;
            return true;
        }
    }

    slotIndex = 1;
    return false;
}

bool SamplerVoicePool::advanceRoundRobinSlot(std::string_view poolId,
                                             int slotCount,
                                             bool usesLegacyScalarKey,
                                             RoundRobinMode mode) noexcept
{
    if (slotCount <= 0)
        return false;

    for (std::size_t index = 0; index < roundRobinPoolCount; ++index)
    {
        if (!sameRoundRobinPool(roundRobinPools[index].key.poolId,
                                roundRobinPools[index].key.slotCount,
                                roundRobinPools[index].key.usesLegacyScalarKey,
                                roundRobinPools[index].key.mode,
                                poolId,
                                slotCount,
                                usesLegacyScalarKey,
                                mode))
            continue;

        if (mode == RoundRobinMode::random)
        {
            roundRobinPools[index].randomState = advanceRandomState(roundRobinPools[index].randomState);
            roundRobinPools[index].nextSlotIndex =
                static_cast<int>(roundRobinPools[index].randomState
                                 % static_cast<std::uint64_t>(slotCount)) + 1;
        }
        else
        {
            roundRobinPools[index].nextSlotIndex =
                advanceRoundRobinSlotIndex(roundRobinPools[index].nextSlotIndex, slotCount);
        }
        return true;
    }

    return false;
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
