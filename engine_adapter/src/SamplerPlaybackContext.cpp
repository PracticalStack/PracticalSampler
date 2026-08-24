#include "drs/engine/SamplerPlaybackContext.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace drs::engine
{
namespace
{
// MSVC's debug STL allocates container proxies even for empty vectors. Keep this immutable
// fallback at process lifetime so an inactive playback context never constructs/destructs six
// vector proxies inside each audio callback.
const CompiledPerformanceProgram emptyPerformanceProgram;
}

SamplerPlaybackContext::SamplerPlaybackContext(PlaybackActivationLane lane)
    : contextLane(lane)
{
    for (auto& value : pendingInstrumentControlValues)
        value.store(0.0, std::memory_order_relaxed);
    for (auto& mask : pendingInstrumentControlMasks)
        mask.store(0, std::memory_order_relaxed);
    reclaimerThread = std::thread([this] { runBackgroundReclaimer(); });
}

SamplerPlaybackContext::~SamplerPlaybackContext()
{
    // Processor shutdown has already stopped audio. Detach every remaining
    // ownership graph before stopping the reclaimer so final destruction never
    // falls back to the caller/message thread.
    for (auto& slot : activationSlots)
    {
        if (slot.model == nullptr)
            continue;
        while (!enqueueBackgroundReclamation(slot))
            waitForBackgroundReclamation(std::chrono::milliseconds(100));
        slot.serial = 0;
    }

    {
        std::lock_guard<std::mutex> lock(reclaimerMutex);
        stopReclaimerRequested = true;
    }
    reclaimerCondition.notify_all();
    if (reclaimerThread.joinable())
        reclaimerThread.join();
}

bool SamplerPlaybackContext::prepare(double outputSampleRate) noexcept
{
    if (!std::isfinite(outputSampleRate) || outputSampleRate <= 0.0)
        return false;

    const auto resetCount = voicePool.activeVoiceCount() + voicePool.releasingVoiceCount();
    counters.resetVoiceCount += resetCount;
    sampleRate = outputSampleRate;
    if (activeDspGeneration != nullptr)
        activeDspGeneration->setControlSampleRate(sampleRate);
    isPrepared = true;
    performanceState.reset();
    if (activeRenderModel != nullptr)
    {
        voicePool.prepare(*activeRenderModel,
                          sampleRate,
                          activationSlots[static_cast<std::size_t>(activeActivationSlot)].serial);
        performanceState.migrateProgram(
            activeRenderModel->getPerformanceProgram(),
            activationSlots[static_cast<std::size_t>(activeActivationSlot)].serial,
            activeRenderModel->usesContinuousDamper(),
            activeRenderModel->getSustainControllerNumber(),
            activeRenderModel->getSustainThreshold());
    }
    else
        voicePool.clearRenderModel();
    eventScratch.clear();
    actionScratch.clear();
    collectFinishedRetirements();
    publishRealtimeDiagnostics();
    return true;
}

bool SamplerPlaybackContext::stageActivation(SamplerRenderModelPtr model)
{
    return stageActivation(std::move(model), {});
}

bool SamplerPlaybackContext::stageActivation(SamplerRenderModelPtr model,
                                             std::shared_ptr<DspRenderGeneration> dspGeneration)
{
    if (model == nullptr || model->getLane() != contextLane)
        return false;
    if (dspGeneration != nullptr && dspGeneration->getSamplerModel() != model)
        return false;
    if (dspGeneration != nullptr)
        dspGeneration->setControlSampleRate(sampleRate > 0.0 ? sampleRate : 48000.0);

    serviceRetirements();
    const auto superseded = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    diagnosticPendingRevision.store(0, std::memory_order_release);
    diagnosticPendingPreparedBuildId.store(0, std::memory_order_release);
    diagnosticPendingPayloadBytes.store(0, std::memory_order_release);
    if (superseded >= 0)
    {
        const auto& slot = activationSlots[static_cast<std::size_t>(superseded)];
        if (!releaseSlotOnMessageThread({ superseded, slot.serial }))
        {
            const auto payload = slot.model != nullptr
                ? slot.model->getRetainedActivationPayload()
                : PlaybackActivationPayloadPtr {};
            diagnosticPendingRevision.store(
                slot.model != nullptr ? slot.model->getRevision() : 0,
                std::memory_order_relaxed);
            diagnosticPendingPreparedBuildId.store(
                slot.model != nullptr ? slot.model->getPreparedBuildId() : 0,
                std::memory_order_relaxed);
            diagnosticPendingPayloadBytes.store(
                payload != nullptr ? payload->retainedPreparedBytes : 0,
                std::memory_order_relaxed);
            pendingActivationSlot.store(superseded, std::memory_order_release);
            return false;
        }
    }

    const auto slotIndex = acquireFreeSlot();
    if (slotIndex < 0)
    {
        // Reclamation stays message-owned; under slot pressure, request a brief audio-owned fade
        // from the oldest retired tail so the next callback can reclaim it deterministically.
        for (std::size_t index = 0; index < retiredActivationCount; ++index)
        {
            const auto& token = retiredActivations[index];
            if (token.slotIndex < 0) continue;
            auto& retired = activationSlots[static_cast<std::size_t>(token.slotIndex)];
            if (retired.serial == token.serial && retired.dspGeneration != nullptr
                && retired.dspGeneration->tailActive())
            {
                retired.dspGeneration->requestRetirementTailFade(
                    static_cast<std::uint32_t>(std::max(1.0, sampleRate * .010)));
                break;
            }
        }
        return false;
    }

    auto& slot = activationSlots[static_cast<std::size_t>(slotIndex)];
    slot.model = std::move(model);
    slot.dspGeneration = std::move(dspGeneration);
    slot.serial = nextActivationSerial++;
    if (nextActivationSerial == 0)
        nextActivationSerial = 1;
    const auto payload = slot.model->getRetainedActivationPayload();
    diagnosticPendingRevision.store(slot.model->getRevision(), std::memory_order_relaxed);
    diagnosticPendingPreparedBuildId.store(slot.model->getPreparedBuildId(), std::memory_order_relaxed);
    diagnosticPendingPayloadBytes.store(payload != nullptr ? payload->retainedPreparedBytes : 0,
                                        std::memory_order_relaxed);
    pendingActivationSlot.store(slotIndex, std::memory_order_release);
    return true;
}

bool SamplerPlaybackContext::publishDspControl(const std::uint64_t generationIdentity,
                                               const std::uint32_t controlIndex,
                                               const double value) noexcept
{
    if (generationIdentity == 0) return false;
    for (const auto& slot : activationSlots)
    {
        if (slot.dspGeneration != nullptr
            && slot.dspGeneration->getControlGenerationIdentity() == generationIdentity)
            return slot.dspGeneration->publishControlValue(generationIdentity, controlIndex, value);
    }
    return false;
}

bool SamplerPlaybackContext::publishDspControlByIdentity(const std::string& slotId,
                                                         const std::string& parameterId,
                                                         const double value) noexcept
{
    DspRenderGeneration* latest = nullptr;
    std::uint64_t latestSerial = 0;
    for (auto& slot : activationSlots)
    {
        if (slot.dspGeneration != nullptr && slot.serial > latestSerial)
        {
            latest = slot.dspGeneration.get();
            latestSerial = slot.serial;
        }
    }
    if (latest == nullptr) return false;
    const auto control = latest->findControlIndex(slotId, parameterId);
    return control.has_value()
        && latest->publishControlValue(latest->getControlGenerationIdentity(), *control, value);
}

bool SamplerPlaybackContext::publishActiveDspControl(const std::uint32_t controlIndex,
                                                     const double value) noexcept
{
    return activeDspGeneration != nullptr
        && activeDspGeneration->publishControlValue(
            activeDspGeneration->getControlGenerationIdentity(), controlIndex, value);
}

bool SamplerPlaybackContext::publishActiveInstrumentControl(const std::size_t controlIndex,
                                                             const double normalizedValue) noexcept
{
    if (activeRenderModel == nullptr || controlIndex >= activeRenderModel->getInstrumentControlBindings().controlCount()
        || !std::isfinite(normalizedValue))
        return false;
    pendingInstrumentControlValues[controlIndex].store(std::clamp(normalizedValue, 0.0, 1.0),
                                                       std::memory_order_relaxed);
    const auto word = controlIndex / 64;
    const auto bit = std::uint64_t { 1 } << (controlIndex % 64);
    pendingInstrumentControlMasks[word].fetch_or(bit, std::memory_order_release);
    return true;
}

void SamplerPlaybackContext::applyPendingInstrumentControls() noexcept
{
    if (activeRenderModel == nullptr)
        return;
    for (std::size_t word = 0; word < pendingInstrumentControlMasks.size(); ++word)
    {
        const auto mask = pendingInstrumentControlMasks[word].exchange(0, std::memory_order_acq_rel);
        auto remaining = mask;
        while (remaining != 0)
        {
            std::size_t bit = 0;
            while (bit < 64 && (remaining & (std::uint64_t { 1 } << bit)) == 0)
                ++bit;
            const auto index = word * 64 + bit;
            if (index < activeRenderModel->getInstrumentControlBindings().controlCount())
                voicePool.setInstrumentControlNormalized(
                    index, pendingInstrumentControlValues[index].load(std::memory_order_acquire));
            remaining &= remaining - 1;
        }
    }
}

bool SamplerPlaybackContext::publishDspNodeBypass(const std::uint64_t generationIdentity,
                                                  const std::uint32_t nodeIndex,
                                                  const bool bypassed) noexcept
{
    for (const auto& slot : activationSlots)
        if (slot.dspGeneration != nullptr
            && slot.dspGeneration->getControlGenerationIdentity() == generationIdentity)
            return slot.dspGeneration->publishNodeBypass(generationIdentity, nodeIndex, bypassed);
    return false;
}

bool SamplerPlaybackContext::publishDspChainBypass(const std::uint64_t generationIdentity,
                                                   const std::uint32_t chainIndex,
                                                   const bool bypassed) noexcept
{
    for (const auto& slot : activationSlots)
        if (slot.dspGeneration != nullptr
            && slot.dspGeneration->getControlGenerationIdentity() == generationIdentity)
            return slot.dspGeneration->publishChainBypass(generationIdentity, chainIndex, bypassed);
    return false;
}

bool SamplerPlaybackContext::cancelPendingActivation()
{
    const auto pending = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    diagnosticPendingRevision.store(0, std::memory_order_release);
    diagnosticPendingPreparedBuildId.store(0, std::memory_order_release);
    diagnosticPendingPayloadBytes.store(0, std::memory_order_release);
    if (pending < 0)
        return false;
    const auto& slot = activationSlots[static_cast<std::size_t>(pending)];
    if (releaseSlotOnMessageThread({ pending, slot.serial }))
        return true;

    const auto payload = slot.model != nullptr
        ? slot.model->getRetainedActivationPayload()
        : PlaybackActivationPayloadPtr {};
    diagnosticPendingRevision.store(
        slot.model != nullptr ? slot.model->getRevision() : 0,
        std::memory_order_relaxed);
    diagnosticPendingPreparedBuildId.store(
        slot.model != nullptr ? slot.model->getPreparedBuildId() : 0,
        std::memory_order_relaxed);
    diagnosticPendingPayloadBytes.store(
        payload != nullptr ? payload->retainedPreparedBytes : 0,
        std::memory_order_relaxed);
    pendingActivationSlot.store(pending, std::memory_order_release);
    return false;
}

bool SamplerPlaybackContext::activatePendingForPreparation() noexcept
{
    if (!isPrepared || activeActivationSlot >= 0)
        return false;
    const auto activated = applyPendingActivationAtBlockBoundary();
    publishRealtimeDiagnostics();
    return activated;
}

std::size_t SamplerPlaybackContext::serviceRetirements()
{
    std::size_t reclaimed = 0;
    RetirementToken token;
    if (hasDeferredMessageRetirement)
    {
        token = deferredMessageRetirement;
        if (!releaseSlotOnMessageThread(token))
            return 0;
        hasDeferredMessageRetirement = false;
        diagnosticRetiredBacklog.fetch_sub(1, std::memory_order_relaxed);
        ++reclaimed;
    }
    while (dequeueRetirement(token))
    {
        const auto renderedBlocks = diagnosticRenderedBlockCount.load(std::memory_order_acquire);
        const auto latencyBlocks = renderedBlocks >= token.enqueuedAtRenderedBlockCount
            ? renderedBlocks - token.enqueuedAtRenderedBlockCount
            : 0;
        lastReclamationLatencyBlocks.store(latencyBlocks, std::memory_order_relaxed);
        auto maximumLatency = maxReclamationLatencyBlocks.load(std::memory_order_relaxed);
        while (maximumLatency < latencyBlocks
               && !maxReclamationLatencyBlocks.compare_exchange_weak(
                   maximumLatency, latencyBlocks, std::memory_order_relaxed))
        {
        }
        const auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
        const auto& payload = slot.model != nullptr
            ? slot.model->getRetainedActivationPayload()
            : PlaybackActivationPayloadPtr {};
        const auto payloadBytes = payload != nullptr ? payload->retainedPreparedBytes : 0;
        if (!releaseSlotOnMessageThread(token))
        {
            deferredMessageRetirement = token;
            hasDeferredMessageRetirement = true;
            break;
        }
        diagnosticRetiredBacklog.fetch_sub(1, std::memory_order_relaxed);
        diagnosticRetiredPayloadBytes.fetch_sub(payloadBytes, std::memory_order_relaxed);
        ++reclaimed;
    }
    reclaimedActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    return reclaimed;
}

bool SamplerPlaybackContext::waitForBackgroundReclamation(
    const std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(reclaimerMutex);
    return reclaimerCondition.wait_for(lock, timeout, [this]
    {
        return reclamationCount == 0 && !reclamationInFlight;
    });
}

SamplerPlaybackContextRenderResult SamplerPlaybackContext::renderBlock(
    SamplerAudioBufferView output,
    SamplerRenderEventView events,
    SamplerRenderControlValues controls) noexcept
{
    SamplerPlaybackContextRenderResult result;
    if (!isPrepared || !output.isValid() || !events.isValid())
        return result;

    result.activationApplied = applyPendingActivationAtBlockBoundary();
    applyPendingInstrumentControls();
    eventScratch.clear();
    actionScratch.clear();
    const auto& program = activeRenderModel == nullptr
        ? emptyPerformanceProgram : activeRenderModel->getPerformanceProgram();
    auto hasPanicReset = false;
    for (std::size_t index = 0; index < events.size; ++index)
    {
        auto raw = events[index];
        if (raw.inputSequence == 0)
            raw.inputSequence = static_cast<std::uint32_t>(index + 1);
        const auto beforeActions = actionScratch.size();
        const auto generation = activeActivationSlot >= 0
            ? activationSlots[static_cast<std::size_t>(activeActivationSlot)].serial : 0;
        if (!performanceState.normalize(raw, generation, program, actionScratch))
        {
            ++counters.droppedEventCount;
            continue;
        }
        for (std::size_t action = beforeActions; action < actionScratch.size(); ++action)
        {
            if (!eventScratch.push(actionScratch.view()[action]))
                ++counters.droppedEventCount;
            else if (actionScratch.view()[action].type == SamplerRenderEventType::reset)
                hasPanicReset = true;
        }
    }

    if (activeRenderModel == nullptr)
    {
        collectFinishedRetirements();
        publishRealtimeDiagnostics();
        return result;
    }

    const auto scopedGraph = activeDspGeneration != nullptr
        && activeDspGeneration->beginScopedRender(output);
    if (scopedGraph)
        activeDspGeneration->setTransport(controls.transport);
    result.voicePool = voicePool.renderBlock(output,
                                              eventScratch.view(),
                                              controls,
                                              scopedGraph ? activeDspGeneration->getRouteOutputViews() : nullptr,
                                              scopedGraph ? activeDspGeneration->getRouteOutputViewCount() : 0);
    result.accepted = result.voicePool.accepted;
    if (result.accepted)
    {
        if (scopedGraph)
        {
            activeDspGeneration->executeScopedGraph(output);
            activeDspGeneration->advanceTail(output.frameCount);
        }
        renderRetiredDspTails(output);
        // Voice reset is sample-positioned inside the voice pool; clear effect memory before the
        // next callback so an emergency reset never carries a delay tail into later audio.
        if (hasPanicReset && activeDspGeneration != nullptr)
            activeDspGeneration->resetEffectState();
        ++counters.renderedBlockCount;
        accumulate(result.voicePool);
    }
    collectFinishedRetirements();
    publishRealtimeDiagnostics();
    return result;
}

void SamplerPlaybackContext::resetAtBlockBoundary() noexcept
{
    const auto resetCount = voicePool.activeVoiceCount() + voicePool.releasingVoiceCount();
    counters.resetVoiceCount += resetCount;
    performanceState.reset();
    if (activeRenderModel != nullptr && activeActivationSlot >= 0)
    {
        const auto generation = activationSlots[
            static_cast<std::size_t>(activeActivationSlot)].serial;
        voicePool.prepare(*activeRenderModel, sampleRate, generation);
        performanceState.migrateProgram(activeRenderModel->getPerformanceProgram(), generation,
                                        activeRenderModel->usesContinuousDamper(),
                                        activeRenderModel->getSustainControllerNumber(),
                                        activeRenderModel->getSustainThreshold());
    }
    else
    {
        voicePool.resetVoices();
    }
    if (activeDspGeneration != nullptr)
        activeDspGeneration->resetEffectState();
    eventScratch.clear();
    collectFinishedRetirements();
    publishRealtimeDiagnostics();
}

void SamplerPlaybackContext::closeAtBlockBoundary() noexcept
{
    resetAtBlockBoundary();
    const auto pending = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    diagnosticPendingRevision.store(0, std::memory_order_release);
    diagnosticPendingPreparedBuildId.store(0, std::memory_order_release);
    diagnosticPendingPayloadBytes.store(0, std::memory_order_release);
    if (pending >= 0)
        addRetiredActivation(pending);
    if (activeActivationSlot >= 0)
        addRetiredActivation(activeActivationSlot);

    activeActivationSlot = -1;
    activeRenderModel = nullptr;
    activeDspGeneration = nullptr;
    activeRevision = 0;
    activePreparedBuildId = 0;
    voicePool.clearRenderModel();
    collectFinishedRetirements();
    publishRealtimeDiagnostics();
}

SamplerPlaybackContextSnapshot SamplerPlaybackContext::getSnapshot() const noexcept
{
    SamplerPlaybackContextSnapshot snapshot;
    snapshot.lane = contextLane;
    for (;;)
    {
        const auto before = diagnosticRealtimeSequence.load(std::memory_order_acquire);
        if ((before & 1u) != 0)
            continue;
        snapshot.prepared = diagnosticPrepared.load(std::memory_order_relaxed);
        snapshot.hasActiveActivation = diagnosticHasActiveActivation.load(std::memory_order_relaxed);
        snapshot.activeRevision = diagnosticActiveRevision.load(std::memory_order_relaxed);
        snapshot.activePreparedBuildId = diagnosticActivePreparedBuildId.load(std::memory_order_relaxed);
        snapshot.activeActivationGeneration
            = diagnosticActiveActivationGeneration.load(std::memory_order_relaxed);
        snapshot.activeActivationPayloadBytes = diagnosticActivePayloadBytes.load(std::memory_order_relaxed);
        snapshot.activeDspNodeCount = diagnosticActiveDspNodeCount.load(std::memory_order_relaxed);
        snapshot.activeDspEffectCount = diagnosticActiveDspEffectCount.load(std::memory_order_relaxed);
        snapshot.activeDspScratchBytes = diagnosticActiveDspScratchBytes.load(std::memory_order_relaxed);
        snapshot.activeDspStateBytes = diagnosticActiveDspStateBytes.load(std::memory_order_relaxed);
        snapshot.activeDspDelayMemoryBytes = diagnosticActiveDspDelayMemoryBytes.load(std::memory_order_relaxed);
        snapshot.activeVoiceCount = diagnosticActiveVoiceCount.load(std::memory_order_relaxed);
        snapshot.releasingVoiceCount = diagnosticReleasingVoiceCount.load(std::memory_order_relaxed);
        snapshot.finishedVoiceCount = diagnosticFinishedVoiceCount.load(std::memory_order_relaxed);
        snapshot.activeGenerationVoiceCount
            = diagnosticActiveGenerationVoiceCount.load(std::memory_order_relaxed);
        snapshot.retiredGenerationVoiceCount
            = diagnosticRetiredGenerationVoiceCount.load(std::memory_order_relaxed);
        snapshot.sustainDeferredVoiceCount
            = diagnosticSustainDeferredVoiceCount.load(std::memory_order_relaxed);
        snapshot.selectedArticulationIndex
            = diagnosticSelectedArticulationIndex.load(std::memory_order_relaxed);
        snapshot.pedalDown = diagnosticPedalDown.load(std::memory_order_relaxed);
        snapshot.heldNoteCount = diagnosticHeldNoteCount.load(std::memory_order_relaxed);
        snapshot.consumedNoteCount = diagnosticConsumedNoteCount.load(std::memory_order_relaxed);
        snapshot.actionOverflowCount = diagnosticActionOverflowCount.load(std::memory_order_relaxed);
        for (std::size_t event = 0; event < snapshot.semanticEventCounts.size(); ++event)
            snapshot.semanticEventCounts[event] = diagnosticSemanticEventCounts[event].load(std::memory_order_relaxed);
        snapshot.counters.renderedBlockCount = diagnosticRenderedBlockCount.load(std::memory_order_relaxed);
        snapshot.counters.startedVoiceCount = diagnosticStartedVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.releasedVoiceCount = diagnosticReleasedVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.completedVoiceCount = diagnosticCompletedVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.stolenVoiceCount = diagnosticStolenVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.generationStealCount
            = diagnosticGenerationStealCount.load(std::memory_order_relaxed);
        snapshot.counters.releasingVoiceStealCount
            = diagnosticReleasingVoiceStealCount.load(std::memory_order_relaxed);
        snapshot.counters.droppedEventCount = diagnosticDroppedEventCount.load(std::memory_order_relaxed);
        snapshot.counters.resetVoiceCount = diagnosticResetVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.dynamicReleaseUpdateCount
            = diagnosticDynamicReleaseUpdateCount.load(std::memory_order_relaxed);
        snapshot.counters.repedalCatchCount
            = diagnosticRepedalCatchCount.load(std::memory_order_relaxed);
        snapshot.counters.crossfadeStartedVoiceCount
            = diagnosticCrossfadeStartedVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.crossfadeOverlapHitCount
            = diagnosticCrossfadeOverlapHitCount.load(std::memory_order_relaxed);
        snapshot.counters.crossfadeFallbackCount
            = diagnosticCrossfadeFallbackCount.load(std::memory_order_relaxed);
        snapshot.counters.roundRobinPoolHitCount
            = diagnosticRoundRobinPoolHitCount.load(std::memory_order_relaxed);
        snapshot.counters.roundRobinPoolMissCount
            = diagnosticRoundRobinPoolMissCount.load(std::memory_order_relaxed);
        snapshot.counters.roundRobinFallbackCount
            = diagnosticRoundRobinFallbackCount.load(std::memory_order_relaxed);
        snapshot.counters.chokedVoiceCount = diagnosticChokedVoiceCount.load(std::memory_order_relaxed);
        snapshot.counters.appliedActivationCount = diagnosticAppliedActivationCount.load(std::memory_order_relaxed);
        snapshot.counters.enqueuedRetirementCount = diagnosticEnqueuedRetirementCount.load(std::memory_order_relaxed);
        if (before == diagnosticRealtimeSequence.load(std::memory_order_acquire))
            break;
    }
    for (;;)
    {
        const auto pending = pendingActivationSlot.load(std::memory_order_acquire);
        if (pending < 0)
        {
            snapshot.hasPendingActivation = false;
            snapshot.pendingRevision = 0;
            snapshot.pendingPreparedBuildId = 0;
            snapshot.pendingActivationPayloadBytes = 0;
            break;
        }
        snapshot.hasPendingActivation = true;
        snapshot.pendingRevision = diagnosticPendingRevision.load(std::memory_order_relaxed);
        snapshot.pendingPreparedBuildId = diagnosticPendingPreparedBuildId.load(std::memory_order_relaxed);
        snapshot.pendingActivationPayloadBytes = diagnosticPendingPayloadBytes.load(std::memory_order_relaxed);
        if (pending == pendingActivationSlot.load(std::memory_order_acquire))
            break;
    }
    snapshot.retiredActivationPayloadBytes
        = diagnosticRetiredPayloadBytes.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog
        = diagnosticRetiredBacklog.load(std::memory_order_acquire);
    snapshot.pendingBackgroundReclamationCount
        = diagnosticPendingBackgroundReclamationCount.load(std::memory_order_relaxed);
    snapshot.completedBackgroundReclamationCount
        = completedBackgroundReclamationCount.load(std::memory_order_relaxed);
    snapshot.lastBackgroundReclamationMicros
        = lastBackgroundReclamationMicros.load(std::memory_order_relaxed);
    snapshot.maxBackgroundReclamationMicros
        = maxBackgroundReclamationMicros.load(std::memory_order_relaxed);
    snapshot.lastBackgroundReclaimerThreadHash
        = lastBackgroundReclaimerThreadHash.load(std::memory_order_relaxed);
    snapshot.counters.reclaimedActivationCount
        = reclaimedActivationCount.load(std::memory_order_relaxed);
    snapshot.counters.lastReclamationLatencyBlocks
        = lastReclamationLatencyBlocks.load(std::memory_order_relaxed);
    snapshot.counters.maxReclamationLatencyBlocks
        = maxReclamationLatencyBlocks.load(std::memory_order_relaxed);
    return snapshot;
}

bool SamplerPlaybackContext::applyPendingActivationAtBlockBoundary() noexcept
{
    const auto pending = pendingActivationSlot.exchange(-1, std::memory_order_acq_rel);
    if (pending < 0)
        return false;
    diagnosticPendingRevision.store(0, std::memory_order_release);
    diagnosticPendingPreparedBuildId.store(0, std::memory_order_release);
    diagnosticPendingPayloadBytes.store(0, std::memory_order_release);

    auto& slot = activationSlots[static_cast<std::size_t>(pending)];
    const auto* model = slot.model.get();
    if (model == nullptr || !voicePool.activateModel(*model, sampleRate, slot.serial))
    {
        addRetiredActivation(pending);
        return false;
    }
    for (auto& mask : pendingInstrumentControlMasks)
        mask.store(0, std::memory_order_release);

    if (activeActivationSlot >= 0)
        addRetiredActivation(activeActivationSlot);
    activeActivationSlot = pending;
    activeRenderModel = model;
    activeDspGeneration = slot.dspGeneration.get();
    activeRevision = model->getRevision();
    activePreparedBuildId = model->getPreparedBuildId();
    performanceState.migrateProgram(model->getPerformanceProgram(), slot.serial,
                                    model->usesContinuousDamper(),
                                    model->getSustainControllerNumber(),
                                    model->getSustainThreshold());
    ++counters.appliedActivationCount;
    return true;
}

void SamplerPlaybackContext::addRetiredActivation(int slotIndex) noexcept
{
    if (slotIndex < 0 || retiredActivationCount >= retiredActivations.size())
        return;
    const auto& slot = activationSlots[static_cast<std::size_t>(slotIndex)];
    retiredActivations[retiredActivationCount++] = { slotIndex, slot.serial };
    const auto& payload = slot.model->getRetainedActivationPayload();
    diagnosticRetiredBacklog.fetch_add(1, std::memory_order_relaxed);
    diagnosticRetiredPayloadBytes.fetch_add(payload != nullptr ? payload->retainedPreparedBytes : 0,
                                            std::memory_order_relaxed);
}

void SamplerPlaybackContext::renderRetiredDspTails(const SamplerAudioBufferView output) noexcept
{
    for (std::size_t index = 0; index < retiredActivationCount; ++index)
    {
        const auto token = retiredActivations[index];
        if (token.slotIndex < 0 || static_cast<std::size_t>(token.slotIndex) >= activationSlots.size())
            continue;
        auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
        if (slot.serial != token.serial || slot.dspGeneration == nullptr || !slot.dspGeneration->tailActive()
            || voicePool.voiceCountUsingModel(slot.model.get()) != 0)
            continue;
        if (slot.dspGeneration->beginRetiredTailRender(output))
        {
            slot.dspGeneration->executeScopedGraph(output);
            slot.dspGeneration->advanceTail(output.frameCount);
        }
    }
}

void SamplerPlaybackContext::collectFinishedRetirements() noexcept
{
    std::size_t index = 0;
    while (index < retiredActivationCount)
    {
        const auto token = retiredActivations[index];
        const auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
        const auto dspTailActive = slot.dspGeneration != nullptr && slot.dspGeneration->tailActive();
        if (voicePool.voiceCountUsingModel(slot.model.get()) != 0 || dspTailActive || !enqueueRetirement(token))
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
    token.enqueuedAtRenderedBlockCount = counters.renderedBlockCount;
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

bool SamplerPlaybackContext::releaseSlotOnMessageThread(RetirementToken token)
{
    if (token.slotIndex < 0
        || static_cast<std::size_t>(token.slotIndex) >= activationSlots.size())
    {
        return false;
    }

    auto& slot = activationSlots[static_cast<std::size_t>(token.slotIndex)];
    if (slot.serial != token.serial || slot.model == nullptr)
        return false;
    if (!enqueueBackgroundReclamation(slot))
        return false;
    slot.serial = 0;
    if (freeActivationSlotCount < freeActivationSlots.size())
        freeActivationSlots[freeActivationSlotCount++] = token.slotIndex;
    return true;
}

bool SamplerPlaybackContext::enqueueBackgroundReclamation(ActivationSlot& slot)
{
    std::lock_guard<std::mutex> lock(reclaimerMutex);
    if (reclamationCount >= reclamationQueue.size())
        return false;

    auto& target = reclamationQueue[reclamationWriteIndex];
    target.model = std::move(slot.model);
    target.dspGeneration = std::move(slot.dspGeneration);
    reclamationWriteIndex = (reclamationWriteIndex + 1) % reclamationQueue.size();
    ++reclamationCount;
    diagnosticPendingBackgroundReclamationCount.store(
        reclamationCount + (reclamationInFlight ? 1u : 0u),
        std::memory_order_relaxed);
    reclaimerCondition.notify_one();
    return true;
}

void SamplerPlaybackContext::runBackgroundReclaimer()
{
    for (;;)
    {
        ReclamationEntry entry;
        {
            std::unique_lock<std::mutex> lock(reclaimerMutex);
            reclaimerCondition.wait(lock, [this]
            {
                return stopReclaimerRequested || reclamationCount != 0;
            });
            if (reclamationCount == 0 && stopReclaimerRequested)
                break;

            auto& queued = reclamationQueue[reclamationReadIndex];
            entry.model = std::move(queued.model);
            entry.dspGeneration = std::move(queued.dspGeneration);
            reclamationReadIndex = (reclamationReadIndex + 1) % reclamationQueue.size();
            --reclamationCount;
            reclamationInFlight = true;
            diagnosticPendingBackgroundReclamationCount.store(
                reclamationCount + 1u, std::memory_order_relaxed);
        }

        // These resets may recursively release hundreds of sample/data-source
        // owners. They deliberately execute only on this reclaimer thread.
        const auto startedAt = std::chrono::steady_clock::now();
        entry.dspGeneration.reset();
        entry.model.reset();
        const auto elapsedMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - startedAt).count());
        lastBackgroundReclamationMicros.store(elapsedMicros, std::memory_order_relaxed);
        completedBackgroundReclamationCount.fetch_add(1, std::memory_order_relaxed);
        lastBackgroundReclaimerThreadHash.store(
            static_cast<std::uint64_t>(std::hash<std::thread::id> {}(
                std::this_thread::get_id())),
            std::memory_order_relaxed);
        auto maximum = maxBackgroundReclamationMicros.load(std::memory_order_relaxed);
        while (maximum < elapsedMicros
               && !maxBackgroundReclamationMicros.compare_exchange_weak(
                   maximum, elapsedMicros, std::memory_order_relaxed))
        {
        }

        {
            std::lock_guard<std::mutex> lock(reclaimerMutex);
            reclamationInFlight = false;
            diagnosticPendingBackgroundReclamationCount.store(
                reclamationCount, std::memory_order_relaxed);
        }
        reclaimerCondition.notify_all();
    }
}

void SamplerPlaybackContext::accumulate(const SamplerVoicePoolRenderResult& result) noexcept
{
    counters.startedVoiceCount += result.render.startedVoiceCount;
    counters.releasedVoiceCount += result.render.releasedVoiceCount;
    counters.completedVoiceCount += result.render.completedVoiceCount;
    counters.stolenVoiceCount += result.render.stolenVoiceCount;
    counters.generationStealCount += result.render.generationStealCount;
    counters.releasingVoiceStealCount += result.render.releasingVoiceStealCount;
    counters.droppedEventCount += result.render.droppedEventCount;
    counters.resetVoiceCount += result.resetVoiceCount;
    counters.dynamicReleaseUpdateCount += result.dynamicReleaseUpdateCount;
    counters.repedalCatchCount += result.repedalCatchCount;
    counters.crossfadeStartedVoiceCount += result.render.crossfadeStartedVoiceCount;
    counters.crossfadeOverlapHitCount += result.render.crossfadeOverlapHitCount;
    counters.crossfadeFallbackCount += result.render.crossfadeFallbackCount;
    counters.roundRobinPoolHitCount += result.render.roundRobinPoolHitCount;
    counters.roundRobinPoolMissCount += result.render.roundRobinPoolMissCount;
    counters.roundRobinFallbackCount += result.render.roundRobinFallbackCount;
    counters.chokedVoiceCount += result.render.chokedVoiceCount;
}

void SamplerPlaybackContext::publishRealtimeDiagnostics() noexcept
{
    diagnosticRealtimeSequence.fetch_add(1, std::memory_order_acq_rel);
    diagnosticPrepared.store(isPrepared, std::memory_order_relaxed);
    diagnosticHasActiveActivation.store(activeActivationSlot >= 0, std::memory_order_relaxed);
    diagnosticActiveRevision.store(activeRevision, std::memory_order_relaxed);
    diagnosticActivePreparedBuildId.store(activePreparedBuildId, std::memory_order_relaxed);
    const auto activeGeneration = activeActivationSlot >= 0
        ? activationSlots[static_cast<std::size_t>(activeActivationSlot)].serial : 0;
    diagnosticActiveActivationGeneration.store(activeGeneration, std::memory_order_relaxed);
    const auto* payload = activeRenderModel != nullptr
        ? activeRenderModel->getRetainedActivationPayload().get()
        : nullptr;
    diagnosticActivePayloadBytes.store(payload != nullptr ? payload->retainedPreparedBytes : 0,
                                       std::memory_order_relaxed);
    const auto* dsp = activeDspGeneration != nullptr ? &activeDspGeneration->getDiagnostics() : nullptr;
    diagnosticActiveDspNodeCount.store(dsp != nullptr ? dsp->nodeCount : 0, std::memory_order_relaxed);
    diagnosticActiveDspEffectCount.store(dsp != nullptr ? dsp->effectCount : 0, std::memory_order_relaxed);
    diagnosticActiveDspScratchBytes.store(dsp != nullptr ? dsp->scratchBytes : 0, std::memory_order_relaxed);
    diagnosticActiveDspStateBytes.store(dsp != nullptr ? dsp->stateBytes : 0, std::memory_order_relaxed);
    diagnosticActiveDspDelayMemoryBytes.store(dsp != nullptr ? dsp->delayMemoryBytes : 0,
                                               std::memory_order_relaxed);
    diagnosticActiveVoiceCount.store(static_cast<std::uint32_t>(voicePool.activeVoiceCount()),
                                     std::memory_order_release);
    diagnosticReleasingVoiceCount.store(static_cast<std::uint32_t>(voicePool.releasingVoiceCount()),
                                        std::memory_order_release);
    diagnosticFinishedVoiceCount.store(static_cast<std::uint32_t>(voicePool.finishedVoiceCount()),
                                       std::memory_order_release);
    diagnosticActiveGenerationVoiceCount.store(
        static_cast<std::uint32_t>(voicePool.voiceCountUsingGeneration(activeGeneration)),
        std::memory_order_release);
    diagnosticRetiredGenerationVoiceCount.store(
        static_cast<std::uint32_t>(voicePool.retiredGenerationVoiceCount()),
        std::memory_order_release);
    diagnosticSustainDeferredVoiceCount.store(
        static_cast<std::uint32_t>(voicePool.sustainDeferredVoiceCount()),
        std::memory_order_release);
    const auto performance = performanceState.getSnapshot();
    diagnosticSelectedArticulationIndex.store(performance.selectedArticulationIndex, std::memory_order_release);
    diagnosticPedalDown.store(performance.pedalDown, std::memory_order_release);
    diagnosticHeldNoteCount.store(performance.heldNoteCount, std::memory_order_release);
    diagnosticConsumedNoteCount.store(performance.consumedNoteCount, std::memory_order_release);
    diagnosticActionOverflowCount.store(performance.actionOverflowCount, std::memory_order_release);
    for (std::size_t event = 0; event < performance.semanticEventCounts.size(); ++event)
        diagnosticSemanticEventCounts[event].store(performance.semanticEventCounts[event], std::memory_order_release);
    diagnosticRenderedBlockCount.store(counters.renderedBlockCount, std::memory_order_release);
    diagnosticStartedVoiceCount.store(counters.startedVoiceCount, std::memory_order_release);
    diagnosticReleasedVoiceCount.store(counters.releasedVoiceCount, std::memory_order_release);
    diagnosticCompletedVoiceCount.store(counters.completedVoiceCount, std::memory_order_release);
    diagnosticStolenVoiceCount.store(counters.stolenVoiceCount, std::memory_order_release);
    diagnosticGenerationStealCount.store(counters.generationStealCount, std::memory_order_release);
    diagnosticReleasingVoiceStealCount.store(counters.releasingVoiceStealCount,
                                             std::memory_order_release);
    diagnosticDroppedEventCount.store(counters.droppedEventCount, std::memory_order_release);
    diagnosticResetVoiceCount.store(counters.resetVoiceCount, std::memory_order_release);
    diagnosticDynamicReleaseUpdateCount.store(counters.dynamicReleaseUpdateCount,
                                               std::memory_order_release);
    diagnosticRepedalCatchCount.store(counters.repedalCatchCount,
                                      std::memory_order_release);
    diagnosticCrossfadeStartedVoiceCount.store(counters.crossfadeStartedVoiceCount,
                                               std::memory_order_release);
    diagnosticCrossfadeOverlapHitCount.store(counters.crossfadeOverlapHitCount,
                                             std::memory_order_release);
    diagnosticCrossfadeFallbackCount.store(counters.crossfadeFallbackCount,
                                           std::memory_order_release);
    diagnosticRoundRobinPoolHitCount.store(counters.roundRobinPoolHitCount,
                                           std::memory_order_release);
    diagnosticRoundRobinPoolMissCount.store(counters.roundRobinPoolMissCount,
                                            std::memory_order_release);
    diagnosticRoundRobinFallbackCount.store(counters.roundRobinFallbackCount,
                                            std::memory_order_release);
    diagnosticChokedVoiceCount.store(counters.chokedVoiceCount, std::memory_order_release);
    diagnosticAppliedActivationCount.store(counters.appliedActivationCount, std::memory_order_release);
    diagnosticEnqueuedRetirementCount.store(counters.enqueuedRetirementCount, std::memory_order_release);
    diagnosticRealtimeSequence.fetch_add(1, std::memory_order_release);
}
} // namespace drs::engine
