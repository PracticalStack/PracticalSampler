#pragma once

#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace drs::engine
{
struct SamplerPlaybackContextCounters
{
    std::uint64_t renderedBlockCount = 0;
    std::uint64_t startedVoiceCount = 0;
    std::uint64_t releasedVoiceCount = 0;
    std::uint64_t completedVoiceCount = 0;
    std::uint64_t stolenVoiceCount = 0;
    std::uint64_t generationStealCount = 0;
    std::uint64_t releasingVoiceStealCount = 0;
    std::uint64_t droppedEventCount = 0;
    std::uint64_t resetVoiceCount = 0;
    std::uint64_t crossfadeStartedVoiceCount = 0;
    std::uint64_t crossfadeOverlapHitCount = 0;
    std::uint64_t crossfadeFallbackCount = 0;
    std::uint64_t appliedActivationCount = 0;
    std::uint64_t enqueuedRetirementCount = 0;
    std::uint64_t reclaimedActivationCount = 0;
    std::uint64_t lastReclamationLatencyBlocks = 0;
    std::uint64_t maxReclamationLatencyBlocks = 0;
};

struct SamplerPlaybackContextSnapshot
{
    PlaybackActivationLane lane = PlaybackActivationLane::preview;
    bool prepared = false;
    bool hasActiveActivation = false;
    bool hasPendingActivation = false;
    std::size_t activeRevision = 0;
    std::size_t pendingRevision = 0;
    std::uint64_t activePreparedBuildId = 0;
    std::uint64_t pendingPreparedBuildId = 0;
    std::uint64_t activeActivationGeneration = 0;
    std::uint64_t activeActivationPayloadBytes = 0;
    std::uint64_t pendingActivationPayloadBytes = 0;
    std::uint64_t retiredActivationPayloadBytes = 0;
    std::uint32_t activeVoiceCount = 0;
    std::uint32_t releasingVoiceCount = 0;
    std::uint32_t finishedVoiceCount = 0;
    std::uint32_t activeGenerationVoiceCount = 0;
    std::uint32_t retiredGenerationVoiceCount = 0;
    std::uint32_t sustainDeferredVoiceCount = 0;
    std::size_t retiredActivationBacklog = 0;
    SamplerPlaybackContextCounters counters;
};

struct SamplerPlaybackContextRenderResult
{
    bool accepted = false;
    bool activationApplied = false;
    SamplerVoicePoolRenderResult voicePool;
};

// One mutable playback lane. Message-owned methods stage and reclaim immutable activations;
// audio-owned methods exchange only primitive slot tokens at block boundaries.
class SamplerPlaybackContext final
{
public:
    static constexpr std::size_t activationSlotCapacity = 4;
    static constexpr std::size_t retirementQueueCapacity = 8;

    explicit SamplerPlaybackContext(PlaybackActivationLane lane) noexcept;
    ~SamplerPlaybackContext() = default;

    SamplerPlaybackContext(const SamplerPlaybackContext&) = delete;
    SamplerPlaybackContext& operator=(const SamplerPlaybackContext&) = delete;
    SamplerPlaybackContext(SamplerPlaybackContext&&) = delete;
    SamplerPlaybackContext& operator=(SamplerPlaybackContext&&) = delete;

    // Audio/control setup. A device restart stops voices but preserves the active activation.
    bool prepare(double outputSampleRate) noexcept;

    // Message-owned activation/reclamation API. Model ownership never moves on the audio thread.
    bool stageActivation(SamplerRenderModelPtr model);
    bool cancelPendingActivation();
    bool activatePendingForPreparation() noexcept;
    std::size_t serviceRetirements();

    // Audio-owned callback API. Pending activation is consumed before the first rendered frame.
    SamplerPlaybackContextRenderResult renderBlock(SamplerAudioBufferView output,
                                                   SamplerRenderEventView events,
                                                   SamplerRenderControlValues controls = {}) noexcept;
    void resetAtBlockBoundary() noexcept;
    void closeAtBlockBoundary() noexcept;

    SamplerPlaybackContextSnapshot getSnapshot() const noexcept;
    const SamplerVoicePool& getVoicePool() const noexcept { return voicePool; }
    const SamplerRenderModel* getActiveRenderModel() const noexcept { return activeRenderModel; }

private:
    struct ActivationSlot
    {
        SamplerRenderModelPtr model;
        std::uint64_t serial = 0;
    };

    struct RetirementToken
    {
        int slotIndex = -1;
        std::uint64_t serial = 0;
        std::uint64_t enqueuedAtRenderedBlockCount = 0;
    };

    bool applyPendingActivationAtBlockBoundary() noexcept;
    void addRetiredActivation(int slotIndex) noexcept;
    void collectFinishedRetirements() noexcept;
    bool enqueueRetirement(RetirementToken token) noexcept;
    bool dequeueRetirement(RetirementToken& token) noexcept;
    int acquireFreeSlot() noexcept;
    void releaseSlotOnMessageThread(RetirementToken token);
    void accumulate(const SamplerVoicePoolRenderResult& result) noexcept;
    void publishRealtimeDiagnostics() noexcept;

    PlaybackActivationLane contextLane;
    SamplerVoicePool voicePool;
    SamplerEventBlock eventScratch;
    double sampleRate = 0.0;
    bool isPrepared = false;
    const SamplerRenderModel* activeRenderModel = nullptr;
    int activeActivationSlot = -1;
    std::size_t activeRevision = 0;
    std::uint64_t activePreparedBuildId = 0;
    std::array<ActivationSlot, activationSlotCapacity> activationSlots {};
    std::array<int, activationSlotCapacity> freeActivationSlots { 0, 1, 2, 3 };
    std::size_t freeActivationSlotCount = activationSlotCapacity;
    std::atomic<int> pendingActivationSlot { -1 };
    std::atomic<std::size_t> diagnosticPendingRevision { 0 };
    std::atomic<std::uint64_t> diagnosticPendingPreparedBuildId { 0 };
    std::atomic<std::uint64_t> diagnosticPendingPayloadBytes { 0 };
    std::atomic<std::uint64_t> diagnosticRealtimeSequence { 0 };
    std::atomic<bool> diagnosticPrepared { false };
    std::atomic<bool> diagnosticHasActiveActivation { false };
    std::atomic<std::size_t> diagnosticActiveRevision { 0 };
    std::atomic<std::uint64_t> diagnosticActivePreparedBuildId { 0 };
    std::atomic<std::uint64_t> diagnosticActiveActivationGeneration { 0 };
    std::atomic<std::uint64_t> diagnosticActivePayloadBytes { 0 };
    std::atomic<std::size_t> diagnosticRetiredBacklog { 0 };
    std::atomic<std::uint64_t> diagnosticRetiredPayloadBytes { 0 };
    std::atomic<std::uint32_t> diagnosticActiveVoiceCount { 0 };
    std::atomic<std::uint32_t> diagnosticReleasingVoiceCount { 0 };
    std::atomic<std::uint32_t> diagnosticFinishedVoiceCount { 0 };
    std::atomic<std::uint32_t> diagnosticActiveGenerationVoiceCount { 0 };
    std::atomic<std::uint32_t> diagnosticRetiredGenerationVoiceCount { 0 };
    std::atomic<std::uint32_t> diagnosticSustainDeferredVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticRenderedBlockCount { 0 };
    std::atomic<std::uint64_t> diagnosticStartedVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticReleasedVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticCompletedVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticStolenVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticGenerationStealCount { 0 };
    std::atomic<std::uint64_t> diagnosticReleasingVoiceStealCount { 0 };
    std::atomic<std::uint64_t> diagnosticDroppedEventCount { 0 };
    std::atomic<std::uint64_t> diagnosticResetVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticCrossfadeStartedVoiceCount { 0 };
    std::atomic<std::uint64_t> diagnosticCrossfadeOverlapHitCount { 0 };
    std::atomic<std::uint64_t> diagnosticCrossfadeFallbackCount { 0 };
    std::atomic<std::uint64_t> diagnosticAppliedActivationCount { 0 };
    std::atomic<std::uint64_t> diagnosticEnqueuedRetirementCount { 0 };
    std::uint64_t nextActivationSerial = 1;
    std::array<RetirementToken, activationSlotCapacity> retiredActivations {};
    std::size_t retiredActivationCount = 0;
    std::array<RetirementToken, retirementQueueCapacity + 1> retirementQueue {};
    std::atomic<std::uint32_t> retirementWriteIndex { 0 };
    std::atomic<std::uint32_t> retirementReadIndex { 0 };
    std::atomic<std::uint64_t> reclaimedActivationCount { 0 };
    std::atomic<std::uint64_t> lastReclamationLatencyBlocks { 0 };
    std::atomic<std::uint64_t> maxReclamationLatencyBlocks { 0 };
    SamplerPlaybackContextCounters counters;
};
} // namespace drs::engine
