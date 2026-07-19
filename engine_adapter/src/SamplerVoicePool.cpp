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
                               double outputSampleRate) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0
        || model.getRoutes().empty() || model.getSamples().empty())
    {
        return false;
    }

    resetVoices();
    nextVoiceId = 1;
    return activateModel(model, outputSampleRate);
}

bool SamplerVoicePool::activateModel(const SamplerRenderModel& model,
                                     double outputSampleRate) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0
        || model.getRoutes().empty() || model.getSamples().empty())
    {
        return false;
    }

    renderModel = &model;
    sampleRate = outputSampleRate;
    return true;
}

void SamplerVoicePool::clearRenderModel() noexcept
{
    resetVoices();
    renderModel = nullptr;
    sampleRate = 0.0;
    nextVoiceId = 1;
}

SamplerVoicePoolRenderResult SamplerVoicePool::renderBlock(SamplerAudioBufferView output,
                                                           SamplerRenderEventView events) noexcept
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
        applyEvent(events[index], result);
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
    }
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

SamplerVoiceSlotSnapshot SamplerVoicePool::getSlotSnapshot(std::size_t index) const noexcept
{
    if (index >= slots.size())
        return {};
    const auto& slot = slots[index];
    return { slot.state,
             slot.voice.getVoiceId(),
             slot.voice.getSourceMidiNote(),
             slot.voice.getEffectiveMidiNote() };
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
            ++result.render.completedVoiceCount;
        }
    }
}

void SamplerVoicePool::applyEvent(const SamplerRenderEvent& event,
                                  SamplerVoicePoolRenderResult& result) noexcept
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

            const auto effectiveVelocity = std::clamp(
                static_cast<int>(std::lround(event.velocity * 127.0f)), 1, 127);
            const auto routeIndex = selectRouteIndex(static_cast<int>(event.midiNote), effectiveVelocity);
            if (routeIndex == std::numeric_limits<std::size_t>::max())
            {
                ++result.render.droppedEventCount;
                return;
            }

            bool stolen = false;
            const auto slotIndex = acquireSlot(stolen);
            auto& slot = slots[slotIndex];
            const auto voiceId = nextVoiceId;
            SamplerVoiceStartRequest request;
            request.voiceId = voiceId;
            request.routeIndex = routeIndex;
            request.sourceMidiNote = static_cast<int>(event.midiNote);
            request.effectiveMidiNote = static_cast<int>(event.midiNote);
            request.effectiveVelocity = effectiveVelocity;
            request.outputSampleRate = sampleRate;
            if (!slot.voice.start(*renderModel, request))
            {
                slot.state = SamplerVoiceSlotState::free;
                ++result.render.droppedEventCount;
                return;
            }

            slot.state = SamplerVoiceSlotState::active;
            ++nextVoiceId;
            if (nextVoiceId == 0)
                nextVoiceId = 1;
            ++result.render.startedVoiceCount;
            if (stolen)
                ++result.render.stolenVoiceCount;
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
                    if (slot.voice.beginRelease())
                    {
                        slot.state = SamplerVoiceSlotState::releasing;
                        ++result.render.releasedVoiceCount;
                    }
                }
            }
            return;

        case SamplerRenderEventType::allNotesOff:
            for (auto& slot : slots)
            {
                if (slot.state == SamplerVoiceSlotState::active)
                {
                    if (slot.voice.beginRelease())
                    {
                        slot.state = SamplerVoiceSlotState::releasing;
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
            }
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

std::size_t SamplerVoicePool::acquireSlot(bool& stolen) noexcept
{
    stolen = false;
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        if (slots[index].state == SamplerVoiceSlotState::free
            || slots[index].state == SamplerVoiceSlotState::finished)
        {
            slots[index].voice.reset();
            return index;
        }
    }

    auto selected = slots.size();
    auto selectedVoiceId = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        if (slots[index].state == SamplerVoiceSlotState::releasing
            && slots[index].voice.getVoiceId() < selectedVoiceId)
        {
            selected = index;
            selectedVoiceId = slots[index].voice.getVoiceId();
        }
    }
    if (selected == slots.size())
    {
        for (std::size_t index = 0; index < slots.size(); ++index)
        {
            if (slots[index].state == SamplerVoiceSlotState::active
                && slots[index].voice.getVoiceId() < selectedVoiceId)
            {
                selected = index;
                selectedVoiceId = slots[index].voice.getVoiceId();
            }
        }
    }

    if (selected == slots.size())
        selected = 0;
    slots[selected].voice.reset();
    stolen = true;
    return selected;
}

void SamplerVoicePool::updateCounts(SamplerVoicePoolRenderResult& result) const noexcept
{
    result.activeVoiceCount = static_cast<std::uint32_t>(activeVoiceCount());
    result.releasingVoiceCount = static_cast<std::uint32_t>(releasingVoiceCount());
    result.finishedVoiceCount = static_cast<std::uint32_t>(finishedVoiceCount());
}
} // namespace drs::engine
