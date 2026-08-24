#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace drs::engine
{
enum class SamplerVoiceLifecycleState : std::uint8_t
{
    idle,
    active,
    releasing,
    finished
};

struct SamplerVoiceStartRequest
{
    std::uint64_t voiceId = 0;
    std::uint64_t triggerId = 1;
    std::size_t routeIndex = 0;
    int sourceMidiNote = 60;
    int effectiveMidiNote = 60;
    int effectiveVelocity = 127;
    std::array<std::uint8_t, 128> controllerValues {};
    double routeGainMultiplier = 1.0;
    double outputSampleRate = 48000.0;
    std::uint64_t activationGeneration = 1;
    bool hasPlaybackRegionOverride = false;
    std::uint64_t playbackStartFrameOverride = 0;
    std::uint64_t playbackEndFrameExclusiveOverride = 0;
    bool loopOverrideEnabled = false;
    std::uint64_t loopStartFrameOverride = 0;
    std::uint64_t loopEndFrameExclusiveOverride = 0;
};

struct SamplerVoiceRenderResult
{
    bool accepted = false;
    std::uint32_t mixedFrameCount = 0;
    std::uint32_t pageMissCount = 0;
    std::uint32_t underrunFrameCount = 0;
    std::uint32_t recoveryCount = 0;
    bool voiceFinished = false;
};

struct SamplerPanGains
{
    float left = 1.0f;
    float right = 1.0f;
};

// Center-preserving linear balance: center is unity in both channels, and each extreme mutes
// only the opposite channel. This preserves the legacy center baseline while making authored pan
// deterministic for the shared renderer.
SamplerPanGains computeSamplerPanGains(double normalizedPan) noexcept;

class SamplerVoice final
{
public:
    static constexpr std::uint32_t compatibilityReleaseSampleCount = 2048;
    static constexpr std::uint32_t pitchModulationRampFrames = 32;
    // Match the default stereo resident-head boundary so page zero is requested
    // immediately instead of waiting until half of the head has already elapsed.
    static constexpr std::uint64_t pageLookAheadFrames = 8192;
    static constexpr std::uint64_t pageIntentCadenceFrames = 256;

    bool start(const SamplerRenderModel& model, const SamplerVoiceStartRequest& request) noexcept;
    bool beginRelease(double overrideReleaseSeconds = 0.0) noexcept;
    bool beginReleaseForControllerValue(std::uint8_t controllerValue) noexcept;
    bool updateDynamicRelease(std::uint8_t controllerNumber,
                              std::uint8_t controllerValue) noexcept;
    bool updateControllerModulation(std::uint8_t controllerNumber,
                                    std::uint8_t controllerValue) noexcept;
    bool updatePitchModulation(std::uint8_t controllerNumber,
                               std::uint8_t controllerValue) noexcept;
    bool updateInstrumentControlModulation(
        const std::array<std::uint8_t, 128>& controllerValues) noexcept;
    bool isSustainDown(const std::array<std::uint8_t, 128>& controllerValues) const noexcept;
    SamplerVoiceRenderResult render(SamplerAudioBufferView output,
                                    std::uint32_t outputStartFrame,
                                    std::uint32_t frameCount) noexcept;
    void reset() noexcept;

    SamplerVoiceLifecycleState getLifecycleState() const noexcept { return lifecycleState; }
    bool isActive() const noexcept { return lifecycleState == SamplerVoiceLifecycleState::active; }
    bool isReleasing() const noexcept { return lifecycleState == SamplerVoiceLifecycleState::releasing; }
    std::uint64_t getVoiceId() const noexcept { return voiceId; }
    std::uint64_t getTriggerId() const noexcept { return triggerId; }
    std::uint64_t getActivationGeneration() const noexcept { return activationGeneration; }
    std::size_t getRouteIndex() const noexcept { return routeIndex; }
    int getSourceMidiNote() const noexcept { return sourceMidiNote; }
    int getEffectiveMidiNote() const noexcept { return effectiveMidiNote; }
    int getEffectiveVelocity() const noexcept { return effectiveVelocity; }
    double getPositionFrames() const noexcept { return positionFrames; }
    double getIncrementFrames() const noexcept { return incrementFrames; }
    double getTargetIncrementFrames() const noexcept { return targetIncrementFrames; }
    double getEffectiveTuningCents() const noexcept { return effectiveTuningCents; }
    float getBaseGain() const noexcept { return baseGain; }
    bool isLoopActive() const noexcept { return loopActive; }
    bool ignoresNoteOff() const noexcept
    {
        return route != nullptr
            && (route->triggerMode == ZoneTriggerMode::oneShot
                || route->loopMode == RegionLoopMode::oneShot);
    }
    SamplerPanGains getPanGains() const noexcept { return panGains; }
    double getEnvelopeHoldSeconds() const noexcept { return envelopeHoldSeconds; }
    double getEnvelopeDecaySeconds() const noexcept { return envelopeDecaySeconds; }
    double getEnvelopeSustainLevel() const noexcept { return envelopeSustainLevel; }
    std::uint32_t getReleaseSamplesRemaining() const noexcept { return releaseSamplesRemaining; }
    std::uint32_t getReleaseSamplesTotal() const noexcept { return releaseSamplesTotal; }
    float getReleaseEnvelopeLevel() const noexcept;
    std::uint32_t getDynamicReleaseUpdateCount() const noexcept
    {
        return dynamicReleaseUpdateCount;
    }
    std::uint32_t getRepedalCatchCount() const noexcept { return repedalCatchCount; }
    int getDamperCurveIndex() const noexcept
    {
        return route != nullptr ? route->damper.releaseCurveIndex : -1;
    }
    int getSustainControllerNumber() const noexcept
    {
        return route != nullptr ? route->damper.sustainControllerNumber
                                : legacySustainControllerNumber;
    }
    int getReleaseControllerNumber() const noexcept
    {
        return route != nullptr ? route->damper.releaseControllerNumber
                                : halfPedalReleaseControllerNumber;
    }
    const SamplerRenderModel* getRenderModel() const noexcept { return renderModel; }

private:
    bool configureRelease(double releaseSeconds,
                          bool retainAuthoredShape,
                          float startingLevel,
                          bool hasControllerValue,
                          std::uint8_t controllerValue) noexcept;
    double dynamicReleaseSeconds(std::uint8_t controllerValue) const noexcept;
    void advancePitchIncrement() noexcept;
    void finish() noexcept;
    double amplitudeEnvelopeLevel() const noexcept;

    SamplerVoiceLifecycleState lifecycleState = SamplerVoiceLifecycleState::idle;
    std::uint64_t voiceId = 0;
    std::uint64_t triggerId = 0;
    std::uint64_t activationGeneration = 0;
    std::size_t routeIndex = 0;
    int sourceMidiNote = 0;
    int effectiveMidiNote = 0;
    int effectiveVelocity = 0;
    const SamplerRenderModel* renderModel = nullptr;
    const SamplerRenderRoute* route = nullptr;
    const SamplerRenderSample* sample = nullptr;
    double positionFrames = 0.0;
    std::uint64_t playbackEndFrame = 0;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::uint64_t loopCrossfadeFrames = 0;
    double incrementFrames = 1.0;
    double targetIncrementFrames = 1.0;
    double pitchIncrementRampStep = 0.0;
    double effectiveTuningCents = 0.0;
    std::uint32_t pitchIncrementRampRemaining = 0;
    double outputSampleRate = 48000.0;
    float baseGain = 0.0f;
    float unmodulatedGain = 0.0f;
    SamplerPanGains panGains;
    bool loopActive = false;
    std::uint32_t releaseSamplesRemaining = 0;
    std::uint32_t releaseSamplesTotal = 0;
    double releaseShape = 0.0;
    float releaseLevelScale = 1.0f;
    bool hasReleaseControllerValue = false;
    std::uint8_t releaseControllerValue = 0;
    std::uint32_t dynamicReleaseUpdateCount = 0;
    std::uint32_t repedalCatchCount = 0;
    double envelopeHoldSeconds = 0.0;
    double envelopeDecaySeconds = 0.0;
    double envelopeSustainLevel = 1.0;
    double envelopeElapsedFrames = 0.0;
    bool underrunning = false;
    std::uint64_t nextLookAheadPublicationFrame = 0;
};
} // namespace drs::engine
