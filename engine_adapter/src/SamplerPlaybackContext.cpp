#include "drs/engine/SamplerPlaybackContext.h"

#include <cmath>
#include <utility>

namespace drs::engine
{
SamplerPlaybackContext::SamplerPlaybackContext(PlaybackActivationLane lane) noexcept
    : contextLane(lane)
{
}

bool SamplerPlaybackContext::prepare(double outputSampleRate) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0)
        return false;

    const auto resetCount = voicePool.activeVoiceCount() + voicePool.releasingVoiceCount();
    counters.resetVoiceCount += resetCount;
    sampleRate = outputSampleRate;
    isPrepared = true;
    if (activeRenderModel != nullptr)
        voicePool.prepare(*activeRenderModel, sampleRate);
    else
        voicePool.clearRenderModel();
    eventScratch.clear();
    collectFinishedRetirements();
    return true;
}

bool SamplerPlaybackContext::stageActivation(SamplerRenderModelPtr model)
{
    if (model == nullptr || model->getLane() != contextLane)
        return false;

    serviceRetirements();
    const auto superseded = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    if (superseded >= 0)
    {
        const auto& slot = activationSlots[static_cast<std::size_t>(superseded)];
        releaseSlotOnMessageThread({ superseded, slot.serial });
    }

    const auto slotIndex = acquireFreeSlot();
    if (slotIndex < 0)
        return false;

    auto& slot = activationSlots[static_cast<std::size_t>(slotIndex)];
    slot.model = std::move(model);
    slot.serial = nextActivationSerial++;
    if (nextActivationSerial == 0)
        nextActivationSerial = 1;
    pendingActivationSlot.store(slotIndex, std::memory_order_release);
    return true;
}

std::size_t SamplerPlaybackContext::serviceRetirements()
{
    std::size_t reclaimed = 0;
    RetirementToken token;
    while (dequeueRetirement(token))
    {
        releaseSlotOnMessageThread(token);
        ++reclaimed;
    }
    reclaimedActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    return reclaimed;
}

SamplerPlaybackContextRenderResult SamplerPlaybackContext::renderBlock(
    SamplerAudioBufferView output,
    SamplerRenderEventView events) noexcept
{
    SamplerPlaybackContextRenderResult result;
    if (!isPrepared || !output.isValid() || !events.isValid())
        return result;

    result.activationApplied = applyPendingActivationAtBlockBoundary();
    eventScratch.clear();
    for (std::size_t index = 0; index < events.size; ++index)
    {
        if (!eventScratch.push(events[index]))
            ++counters.droppedEventCount;
    }

    if (activeRenderModel == nullptr)
    {
        collectFinishedRetirements();
        return result;
    }

    result.voicePool = voicePool.renderBlock(output, eventScratch.view());
    result.accepted = result.voicePool.accepted;
    if (result.accepted)
    {
        ++counters.renderedBlockCount;
        accumulate(result.voicePool);
    }
    collectFinishedRetirements();
    return result;
}

void SamplerPlaybackContext::resetAtBlockBoundary() noexcept
{
    const auto resetCount = voicePool.activeVoiceCount() + voicePool.releasingVoiceCount();
    counters.resetVoiceCount += resetCount;
    voicePool.resetVoices();
    eventScratch.clear();
    collectFinishedRetirements();
}

void SamplerPlaybackContext::closeAtBlockBoundary() noexcept
{
    resetAtBlockBoundary();
    const auto pending = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    if (pending >= 0)
        addRetiredActivation(pending);
    if (activeActivationSlot >= 0)
        addRetiredActivation(activeActivationSlot);

    activeActivationSlot = -1;
    activeRenderModel = nullptr;
    activeRevision = 0;
    activePreparedBuildId = 0;
    voicePool.clearRenderModel();
    collectFinishedRetirements();
}

SamplerPlaybackContextSnapshot SamplerPlaybackContext::getSnapshot() const noexcept
{
    SamplerPlaybackContextSnapshot snapshot;
    snapshot.lane = contextLane;
    snapshot.prepared = isPrepared;
    snapshot.hasActiveActivation = activeActivationSlot >= 0;
    snapshot.hasPendingActivation = pendingActivationSlot.load(std::memory_order_acquire) >= 0;
    snapshot.activeRevision = activeRevision;
    snapshot.activePreparedBuildId = activePreparedBuildId;
    snapshot.activeVoiceCount = static_cast<std::uint32_t>(voicePool.activeVoiceCount());
    snapshot.releasingVoiceCount = static_cast<std::uint32_t>(voicePool.releasingVoiceCount());
    snapshot.finishedVoiceCount = static_cast<std::uint32_t>(voicePool.finishedVoiceCount());
    snapshot.retiredActivationBacklog = retiredActivationCount;
    const auto read = retirementReadIndex.load(std::memory_order_acquire);
    const auto write = retirementWriteIndex.load(std::memory_order_acquire);
    const auto queueSize = static_cast<std::uint32_t>(retirementQueue.size());
    snapshot.retiredActivationBacklog += (write + queueSize - read) % queueSize;
    snapshot.counters = counters;
    snapshot.counters.reclaimedActivationCount
        = reclaimedActivationCount.load(std::memory_order_relaxed);
    return snapshot;
}

bool SamplerPlaybackContext::applyPendingActivationAtBlockBoundary() noexcept
{
    const auto pending = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    if (pending < 0)
        return false;

    auto& slot = activationSlots[static_cast<std::size_t>(pending)];
    const auto* model = slot.model.get();
    if (model == nullptr || !voicePool.activateModel(*model, sampleRate))
    {
        addRetiredActivation(pending);
        return false;
    }

    if (activeActivationSlot >= 0)
        addRetiredActivation(activeActivationSlot);
    activeActivationSlot = pending;
    activeRenderModel = model;
    activeRevision = model->getRevision();
    activePreparedBuildId = model->getPreparedBuildId();
    ++counters.appliedActivationCount;
    return true;
}

void SamplerPlaybackContext::addRetiredActivation(int slotIndex) noexcept
{
    if (slotIndex < 0 || retiredActivationCount >= retiredActivations.size())
        return;
    const auto& slot = activationSlots[static_cast<std::size_t>(slotIndex)];
    retiredActivations[retiredActivationCount++] = { slotIndex, slot.serial };
}

void SamplerPlaybackContext::collectFinishedRetirements() noexcept
{
    std::size_t index = 0;
    while (index < retiredActivationCount)
    {
        const auto token = retiredActivations[index];
        const auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
        if (voicePool.voiceCountUsingModel(slot.model.get()) != 0 || !enqueueRetirement(token))
        {
            ++index;
            continue;
        }

        ++counters.enqueuedRetirementCount;
        --retiredActivationCount;
        retiredActivations[index] = retiredActivations[retiredActivationCount];
        retiredActivations[retiredActivationCount] = {};
    }
}

bool SamplerPlaybackContext::enqueueRetirement(RetirementToken token) noexcept
{
    const auto write = retirementWriteIndex.load(std::memory_order_relaxed);
    const auto next = static_cast<std::uint32_t>((write + 1u) % retirementQueue.size());
    if (next == retirementReadIndex.load(std::memory_order_acquire))
        return false;
    retirementQueue[write] = token;
    retirementWriteIndex.store(next, std::memory_order_release);
    return true;
}

bool SamplerPlaybackContext::dequeueRetirement(RetirementToken& token) noexcept
{
    const auto read = retirementReadIndex.load(std::memory_order_relaxed);
    if (read == retirementWriteIndex.load(std::memory_order_acquire))
        return false;
    token = retirementQueue[read];
    retirementReadIndex.store(static_cast<std::uint32_t>((read + 1u) % retirementQueue.size()),
                              std::memory_order_release);
    return true;
}

int SamplerPlaybackContext::acquireFreeSlot() noexcept
{
    if (freeActivationSlotCount == 0)
        return -1;
    return freeActivationSlots[--freeActivationSlotCount];
}

void SamplerPlaybackContext::releaseSlotOnMessageThread(RetirementToken token)
{
    if (token.slotIndex < 0
        || static_cast<std::size_t>(token.slotIndex) >= activationSlots.size())
    {
        return;
    }

    auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
    if (slot.serial != token.serial || slot.model == nullptr)
        return;
    slot.model.reset();
    slot.serial = 0;
    if (freeActivationSlotCount < freeActivationSlots.size())
        freeActivationSlots[freeActivationSlotCount++] = token.slotIndex;
}

void SamplerPlaybackContext::accumulate(const SamplerVoicePoolRenderResult& result) noexcept
{
    counters.startedVoiceCount += result.render.startedVoiceCount;
    counters.releasedVoiceCount += result.render.releasedVoiceCount;
    counters.completedVoiceCount += result.render.completedVoiceCount;
    counters.stolenVoiceCount += result.render.stolenVoiceCount;
    counters.droppedEventCount += result.render.droppedEventCount;
    counters.resetVoiceCount += result.resetVoiceCount;
}
} // namespace drs::engine
