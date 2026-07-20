#include "drs/engine/SamplerVoicePool.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
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
            // selected voice and must not make an otherwise playable authored zone disappear.
            // The effective-velocity fallback preserves legacy fixed-layer presets when the
            // physical velocity has no route in the selected articulation.
            auto routeIndex = selectRouteIndex(sourceMidiNote, eventVelocity);
            if (routeIndex == std::numeric_limits<std::size_t>::max()
                && effectiveVelocity != eventVelocity)
            {
                routeIndex = selectRouteIndex(sourceMidiNote, effectiveVelocity);
            }
            if (routeIndex == std::numeric_limits<std::size_t>::max())
            {
                ++result.render.droppedEventCount;
                return;
            }

            bool stolen = false;
            bool generationStolen = false;
            bool releasingStolen = false;
            const auto slotIndex = acquireSlot(stolen, generationStolen, releasingStolen);
            auto& slot = slots[slotIndex];
            const auto voiceId = nextVoiceId;
            SamplerVoiceStartRequest request;
            request.voiceId = voiceId;
            request.activationGeneration = activeGeneration;
            request.routeIndex = routeIndex;
            request.sourceMidiNote = sourceMidiNote;
            request.effectiveMidiNote = effectiveMidiNote;
            request.effectiveVelocity = effectiveVelocity;
            request.outputSampleRate = sampleRate;
            if (!slot.voice.start(*renderModel, request))
            {
                slot.state = SamplerVoiceSlotState::free;
                slot.sustainDeferred = false;
                ++result.render.droppedEventCount;
                return;
            }

            slot.state = SamplerVoiceSlotState::active;
            slot.sustainDeferred = false;
            ++nextVoiceId;
            if (nextVoiceId == 0)
                nextVoiceId = 1;
            ++result.render.startedVoiceCount;
            if (stolen)
                ++result.render.stolenVoiceCount;
            if (generationStolen)
                ++result.render.generationStealCount;
            if (releasingStolen)
                ++result.render.releasingVoiceStealCount;
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

std::size_t SamplerVoicePool::selectRouteIndex(int midiNote, int velocity) const noexcept
{
    auto selected = std::numeric_limits<std::size_t>::max();
    const auto& routes = renderModel->getRoutes();
    for (std::size_t index = 0; index < routes.size(); ++index)
    {
        const auto& candidate = routes[index];
        if (midiNote < candidate.keyLow || midiNote > candidate.keyHigh
            || velocity < candidate.velocityLow || velocity > candidate.velocityHigh)
        {
            continue;
        }

        if (selected == std::numeric_limits<std::size_t>::max())
        {
            selected = index;
            continue;
        }

        const auto& current = routes[selected];
        const auto candidateRootDistance = std::abs(candidate.rootKey - midiNote);
        const auto currentRootDistance = std::abs(current.rootKey - midiNote);
        const auto candidateKeySpan = candidate.keyHigh - candidate.keyLow;
        const auto currentKeySpan = current.keyHigh - current.keyLow;
        const auto candidateVelocitySpan = candidate.velocityHigh - candidate.velocityLow;
        const auto currentVelocitySpan = current.velocityHigh - current.velocityLow;
        if (candidateRootDistance < currentRootDistance
            || (candidateRootDistance == currentRootDistance && candidateKeySpan < currentKeySpan)
            || (candidateRootDistance == currentRootDistance && candidateKeySpan == currentKeySpan
                && candidateVelocitySpan < currentVelocitySpan)
            || (candidateRootDistance == currentRootDistance && candidateKeySpan == currentKeySpan
                && candidateVelocitySpan == currentVelocitySpan && candidate.zoneId < current.zoneId))
        {
            selected = index;
        }
    }
    return selected;
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
