#include "drs/engine/SamplerVoice.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
float computeReleaseEnvelope(const std::uint32_t samplesRemaining,
                             const std::uint32_t samplesTotal,
                             const double authoredShape) noexcept
{
    if (samplesTotal == 0)
        return 1.0f;

    const auto linearLevel = static_cast<double>(samplesRemaining)
        / static_cast<double>(samplesTotal);
    if (!std::isfinite(authoredShape) || std::abs(authoredShape) < 1.0e-6)
        return static_cast<float>(linearLevel);

    // Negative values produce the fast initial decay used by SFZ/ARIA;
    // positive values retain more level until late in the release.
    const auto shape = std::clamp(authoredShape, -60.0, 60.0);
    const auto curvedLevel = std::expm1(-shape * linearLevel) / std::expm1(-shape);
    return static_cast<float>(std::clamp(curvedLevel, 0.0, 1.0));
}

double controllerCurveValue(const RuntimeControllerModulation& modulation,
                            const std::array<std::uint8_t, 128>& controllerValues) noexcept
{
    if (!modulation.isActive())
        return 0.0;
    const auto value = controllerValues[static_cast<std::size_t>(modulation.controllerNumber)];
    if (modulation.curveIndex < 0)
        return static_cast<double>(value) / 127.0;
    return std::clamp(modulation.curve[value], 0.0, 1.0);
}

double amplitudeControllerScale(const RuntimeControllerModulation& modulation,
                                const std::array<std::uint8_t, 128>& controllerValues) noexcept
{
    if (!modulation.isActive())
        return 1.0;
    return std::clamp(modulation.amount / 100.0
                          * controllerCurveValue(modulation, controllerValues),
                      0.0, 1.0);
}

double tuningControllerCents(const RuntimeControllerModulation& modulation,
                             const std::array<std::uint8_t, 128>& controllerValues) noexcept
{
    if (!modulation.isActive())
        return 0.0;
    return modulation.amount * controllerCurveValue(modulation, controllerValues);
}

double pitchIncrementFor(const SamplerRenderRoute& route,
                         const SamplerRenderSample& sample,
                         const int effectiveMidiNote,
                         const double outputSampleRate,
                         const double tuningCents) noexcept
{
    const auto pitchRatio = std::pow(
        2.0,
        ((static_cast<double>(effectiveMidiNote - route.rootKey) * 100.0) + tuningCents) / 1200.0);
    return pitchRatio * (sample.sampleRate / outputSampleRate);
}
} // namespace

SamplerPanGains computeSamplerPanGains(double normalizedPan) noexcept
{
    const auto pan = static_cast<float>(std::clamp(normalizedPan, -1.0, 1.0));
    return { pan > 0.0f ? 1.0f - pan : 1.0f,
             pan < 0.0f ? 1.0f + pan : 1.0f };
}

bool SamplerVoice::start(const SamplerRenderModel& model,
                         const SamplerVoiceStartRequest& request) noexcept
{
    reset();
    if (request.voiceId == 0
        || request.triggerId == 0
        || request.activationGeneration == 0
        || request.routeIndex >= model.getRoutes().size()
        || request.sourceMidiNote < 0 || request.sourceMidiNote > 127
        || request.effectiveMidiNote < 0 || request.effectiveMidiNote > 127
        || request.effectiveVelocity < 1 || request.effectiveVelocity > 127
        || !std::isfinite(request.routeGainMultiplier) || request.routeGainMultiplier <= 0.0
        || !std::isfinite(request.outputSampleRate) || request.outputSampleRate <= 0.0)
    {
        return false;
    }

    const auto& selectedRoute = model.getRoutes()[request.routeIndex];
    if (selectedRoute.preparedSampleIndex >= model.getSamples().size())
        return false;

    const auto& selectedSample = model.getSamples()[selectedRoute.preparedSampleIndex];
    const auto resolvedPlaybackEnd = resolveSampleEndFrame(selectedRoute.sampleEndFrame,
                                                            selectedSample.frameCount);
    const auto playbackStart = request.hasPlaybackRegionOverride
        ? request.playbackStartFrameOverride : selectedRoute.sampleStartFrame;
    const auto playbackEnd = request.hasPlaybackRegionOverride
        ? request.playbackEndFrameExclusiveOverride : resolvedPlaybackEnd;
    if (selectedSample.dataSource == nullptr
        || selectedSample.frameCount == 0
        || playbackStart >= playbackEnd || playbackEnd > selectedSample.frameCount
        || !std::isfinite(selectedSample.sampleRate) || selectedSample.sampleRate <= 0.0)
    {
        return false;
    }

    const auto effectiveTuning = selectedRoute.fineTuneCents
        + tuningControllerCents(selectedRoute.tuningModulation, request.controllerValues);
    const auto increment = pitchIncrementFor(selectedRoute, selectedSample,
                                             request.effectiveMidiNote,
                                             request.outputSampleRate,
                                             effectiveTuning);
    // Native amp_veltrack law: 0% is velocity-independent, 100% retains the
    // historical linear velocity gain, and intermediate values use a smooth power curve.
    const auto normalizedVelocity = static_cast<double>(request.effectiveVelocity) / 127.0;
    const auto velocityExponent = std::clamp(selectedRoute.amplitudeVelocityTracking, 0.0, 100.0) / 100.0;
    const auto gain = std::pow(normalizedVelocity, velocityExponent)
        * request.routeGainMultiplier
        * std::pow(10.0, selectedRoute.gainDb / 20.0);
    if (!std::isfinite(increment) || increment <= 0.0 || !std::isfinite(gain))
        return false;

    lifecycleState = SamplerVoiceLifecycleState::active;
    voiceId = request.voiceId;
    triggerId = request.triggerId;
    activationGeneration = request.activationGeneration;
    routeIndex = request.routeIndex;
    sourceMidiNote = request.sourceMidiNote;
    effectiveMidiNote = request.effectiveMidiNote;
    effectiveVelocity = request.effectiveVelocity;
    renderModel = &model;
    route = &selectedRoute;
    sample = &selectedSample;
    positionFrames = static_cast<double>(playbackStart);
    playbackEndFrame = playbackEnd;
    nextLookAheadPublicationFrame = playbackStart;
    incrementFrames = increment;
    targetIncrementFrames = increment;
    effectiveTuningCents = effectiveTuning;
    outputSampleRate = request.outputSampleRate;
    unmodulatedGain = static_cast<float>(gain);
    baseGain = unmodulatedGain * static_cast<float>(amplitudeControllerScale(
        selectedRoute.amplitudeModulation, request.controllerValues));
    panGains = computeSamplerPanGains(selectedRoute.pan);
    const auto& authoredEnvelope = selectedRoute.amplitudeEnvelope;
    const auto holdCurveValue = controllerCurveValue(authoredEnvelope.holdModulation,
                                                      request.controllerValues);
    const auto decayCurveValue = controllerCurveValue(authoredEnvelope.decayModulation,
                                                       request.controllerValues);
    const auto sustainCurveValue = controllerCurveValue(authoredEnvelope.sustainModulation,
                                                        request.controllerValues);
    envelopeHoldSeconds = std::max(0.0, authoredEnvelope.holdSeconds
        + authoredEnvelope.holdModulation.amount * holdCurveValue);
    envelopeDecaySeconds = std::max(0.0, authoredEnvelope.decaySeconds
        + authoredEnvelope.decayModulation.amount * decayCurveValue);
    envelopeSustainLevel = std::clamp(authoredEnvelope.sustainLevel
        + (authoredEnvelope.sustainModulation.amount / 100.0) * sustainCurveValue,
        0.0, 1.0);
    envelopeElapsedFrames = 0.0;
    const auto effectiveLoopMode = effectiveRegionLoopMode(selectedRoute.loopMode,
                                                            selectedRoute.loopEnabled);
    loopStartFrame = request.hasPlaybackRegionOverride
        ? request.loopStartFrameOverride : selectedRoute.loopStartFrame;
    loopEndFrame = request.hasPlaybackRegionOverride
        ? request.loopEndFrameExclusiveOverride : selectedRoute.loopEndFrame;
    loopActive = (request.hasPlaybackRegionOverride
            ? request.loopOverrideEnabled : regionLoopModeLoops(effectiveLoopMode))
        && loopStartFrame < loopEndFrame
        && loopEndFrame > playbackStart
        && loopEndFrame <= playbackEndFrame;
    const auto loopLength = loopActive ? loopEndFrame - loopStartFrame : 0;
    loopCrossfadeFrames = loopActive
        ? std::min(selectedRoute.loopCrossfadeFrames, loopLength / 2) : 0;
    return true;
}

bool SamplerVoice::beginRelease(const double overrideReleaseSeconds) noexcept
{
    if (lifecycleState == SamplerVoiceLifecycleState::releasing)
        return false;
    if (lifecycleState != SamplerVoiceLifecycleState::active)
        return false;

    if (route != nullptr && route->loopMode == RegionLoopMode::loopSustain)
        loopActive = false;

    const auto releaseSeconds = overrideReleaseSeconds > 0.0
        ? overrideReleaseSeconds : (route != nullptr ? route->releaseSeconds : 0.0);
    const auto usesAuthoredRelease = overrideReleaseSeconds <= 0.0
        && route != nullptr && route->releaseSeconds > 0.0;
    return configureRelease(releaseSeconds, usesAuthoredRelease,
                            static_cast<float>(amplitudeEnvelopeLevel()), false, 0);
}

bool SamplerVoice::beginReleaseForControllerValue(const std::uint8_t controllerValue) noexcept
{
    if (lifecycleState != SamplerVoiceLifecycleState::active)
        return false;
    if (route != nullptr && route->loopMode == RegionLoopMode::loopSustain)
        loopActive = false;
    if (route == nullptr || !route->damper.dynamicRelease)
        return beginRelease();
    return configureRelease(dynamicReleaseSeconds(controllerValue), true,
                            static_cast<float>(amplitudeEnvelopeLevel()),
                            true, controllerValue);
}

bool SamplerVoice::configureRelease(const double releaseSeconds,
                                    const bool retainAuthoredShape,
                                    const float startingLevel,
                                    const bool hasControllerValue,
                                    const std::uint8_t controllerValue) noexcept
{
    lifecycleState = SamplerVoiceLifecycleState::releasing;
    releaseShape = retainAuthoredShape && route != nullptr && std::isfinite(route->releaseShape)
        ? route->releaseShape : 0.0;
    releaseLevelScale = std::clamp(startingLevel, 0.0f, 1.0f);
    hasReleaseControllerValue = hasControllerValue;
    releaseControllerValue = controllerValue;
    if (releaseSeconds > 0.0 && std::isfinite(outputSampleRate) && outputSampleRate > 0.0)
    {
        releaseSamplesTotal = static_cast<std::uint32_t>(
            std::max(1ll, static_cast<long long>(std::llround(releaseSeconds * outputSampleRate))));
    }
    else
    {
        releaseSamplesTotal = compatibilityReleaseSampleCount;
    }
    releaseSamplesRemaining = releaseSamplesTotal;
    return true;
}

double SamplerVoice::dynamicReleaseSeconds(const std::uint8_t controllerValue) const noexcept
{
    if (route == nullptr || !route->damper.dynamicRelease)
        return route != nullptr ? route->releaseSeconds : 0.0;
    const auto curveValue = route->damper.releaseCurve[controllerValue];
    return std::clamp(route->releaseSeconds
                          + route->damper.releaseAmountSeconds * curveValue,
                      minimumDynamicReleaseSeconds,
                      maximumDynamicReleaseSeconds);
}

float SamplerVoice::getReleaseEnvelopeLevel() const noexcept
{
    if (!isReleasing())
        return lifecycleState == SamplerVoiceLifecycleState::active ? 1.0f : 0.0f;
    return releaseLevelScale
        * computeReleaseEnvelope(releaseSamplesRemaining, releaseSamplesTotal, releaseShape);
}

bool SamplerVoice::updateDynamicRelease(const std::uint8_t controllerNumber,
                                        const std::uint8_t controllerValue) noexcept
{
    if (!isReleasing() || route == nullptr || !route->damper.dynamicRelease
        || controllerNumber != route->damper.releaseControllerNumber
        || (hasReleaseControllerValue && releaseControllerValue == controllerValue))
        return false;

    const auto currentLevel = getReleaseEnvelopeLevel();
    const auto isRepedalCatch = hasReleaseControllerValue
        && controllerValue > releaseControllerValue;
    const auto releaseSeconds = dynamicReleaseSeconds(controllerValue);
    const auto samples = static_cast<std::uint32_t>(std::max(
        1ll, static_cast<long long>(std::llround(releaseSeconds * outputSampleRate))));
    releaseSamplesTotal = samples;
    releaseSamplesRemaining = samples;
    releaseLevelScale = currentLevel;
    hasReleaseControllerValue = true;
    releaseControllerValue = controllerValue;
    ++dynamicReleaseUpdateCount;
    repedalCatchCount += isRepedalCatch ? 1u : 0u;
    return true;
}

bool SamplerVoice::updateControllerModulation(const std::uint8_t controllerNumber,
                                              const std::uint8_t controllerValue) noexcept
{
    if (!isActive() || route == nullptr
        || !route->amplitudeModulation.isActive()
        || route->amplitudeModulation.controllerNumber != controllerNumber)
        return false;

    std::array<std::uint8_t, 128> values {};
    values[controllerNumber] = controllerValue;
    baseGain = unmodulatedGain * static_cast<float>(amplitudeControllerScale(
        route->amplitudeModulation, values));
    return true;
}

bool SamplerVoice::updatePitchModulation(const std::uint8_t controllerNumber,
                                         const std::uint8_t controllerValue) noexcept
{
    if (!isActive() || route == nullptr || sample == nullptr
        || !route->tuningModulation.isActive()
        || route->tuningModulation.controllerNumber != controllerNumber
        || !std::isfinite(outputSampleRate) || outputSampleRate <= 0.0)
        return false;

    std::array<std::uint8_t, 128> values {};
    values[controllerNumber] = controllerValue;
    const auto tuning = route->fineTuneCents
        + tuningControllerCents(route->tuningModulation, values);
    const auto increment = pitchIncrementFor(*route, *sample, effectiveMidiNote,
                                             outputSampleRate, tuning);
    if (!std::isfinite(increment) || increment <= 0.0)
        return false;

    effectiveTuningCents = tuning;
    targetIncrementFrames = increment;
    if (pitchModulationRampFrames == 0)
    {
        incrementFrames = increment;
        pitchIncrementRampStep = 0.0;
        pitchIncrementRampRemaining = 0;
    }
    else
    {
        pitchIncrementRampRemaining = pitchModulationRampFrames;
        pitchIncrementRampStep = (targetIncrementFrames - incrementFrames)
            / static_cast<double>(pitchIncrementRampRemaining);
    }
    return true;
}

void SamplerVoice::advancePitchIncrement() noexcept
{
    positionFrames += incrementFrames;
    if (pitchIncrementRampRemaining == 0)
        return;

    incrementFrames += pitchIncrementRampStep;
    if (--pitchIncrementRampRemaining == 0)
    {
        incrementFrames = targetIncrementFrames;
        pitchIncrementRampStep = 0.0;
    }
}

double SamplerVoice::amplitudeEnvelopeLevel() const noexcept
{
    if (envelopeHoldSeconds > 0.0
        && envelopeElapsedFrames < envelopeHoldSeconds * outputSampleRate)
        return 1.0;

    const auto decayFrames = envelopeDecaySeconds * outputSampleRate;
    const auto decayStart = envelopeHoldSeconds * outputSampleRate;
    if (decayFrames > 0.0 && envelopeElapsedFrames < decayStart + decayFrames)
    {
        const auto progress = std::clamp((envelopeElapsedFrames - decayStart) / decayFrames,
                                         0.0, 1.0);
        return 1.0 + (envelopeSustainLevel - 1.0) * progress;
    }
    return envelopeSustainLevel;
}

bool SamplerVoice::isSustainDown(
    const std::array<std::uint8_t, 128>& controllerValues) const noexcept
{
    if (route == nullptr)
        return false;
    const auto controller = static_cast<std::size_t>(std::clamp(
        route->damper.sustainControllerNumber, 0, 127));
    return static_cast<double>(controllerValues[controller]) >= route->damper.sustainThreshold;
}

SamplerVoiceRenderResult SamplerVoice::render(SamplerAudioBufferView output,
                                              std::uint32_t outputStartFrame,
                                              std::uint32_t frameCount) noexcept
{
    SamplerVoiceRenderResult result;
    if (!output.isValid()
        || outputStartFrame > output.frameCount
        || frameCount > output.frameCount - outputStartFrame)
    {
        return result;
    }

    result.accepted = true;
    if ((!isActive() && !isReleasing())
        || frameCount == 0 || renderModel == nullptr || route == nullptr || sample == nullptr)
    {
        result.voiceFinished = lifecycleState == SamplerVoiceLifecycleState::finished;
        return result;
    }

    const auto sourceChannelCount = static_cast<std::size_t>(sample->channelCount);
    for (std::uint32_t outputFrame = 0; outputFrame < frameCount; ++outputFrame)
    {
        if (positionFrames >= static_cast<double>(playbackEndFrame))
        {
            finish();
            break;
        }

        const auto frameIndex = static_cast<std::size_t>(positionFrames);
        auto nextFrameIndex = std::min(frameIndex + 1,
                                       static_cast<std::size_t>(playbackEndFrame - 1));
        if (loopActive
            && frameIndex < loopEndFrame
            && nextFrameIndex >= loopEndFrame)
        {
            nextFrameIndex = static_cast<std::size_t>(loopStartFrame);
        }
        const auto fraction = static_cast<float>(positionFrames - static_cast<double>(frameIndex));
        const auto currentView = sample->dataSource->acquireFrameView(frameIndex, 1);
        const auto nextView = sample->dataSource->acquireFrameView(nextFrameIndex, 1);
        const auto crossfadeStartFrame = loopEndFrame - loopCrossfadeFrames;
        const auto crossfading = loopActive && loopCrossfadeFrames != 0
            && positionFrames >= static_cast<double>(crossfadeStartFrame)
            && positionFrames < static_cast<double>(loopEndFrame);
        auto crossfadePosition = static_cast<double>(loopStartFrame);
        std::size_t crossfadeFrameIndex = 0;
        std::size_t crossfadeNextFrameIndex = 0;
        SampleFrameView crossfadeCurrentView;
        SampleFrameView crossfadeNextView;
        if (crossfading)
        {
            crossfadePosition += positionFrames - static_cast<double>(crossfadeStartFrame);
            crossfadeFrameIndex = static_cast<std::size_t>(crossfadePosition);
            crossfadeNextFrameIndex = crossfadeFrameIndex + 1;
            if (crossfadeNextFrameIndex >= loopEndFrame)
                crossfadeNextFrameIndex = static_cast<std::size_t>(loopStartFrame);
            crossfadeCurrentView = sample->dataSource->acquireFrameView(crossfadeFrameIndex, 1);
            crossfadeNextView = sample->dataSource->acquireFrameView(crossfadeNextFrameIndex, 1);
        }
        if (currentView.status != SampleFrameViewStatus::ready
            || nextView.status != SampleFrameViewStatus::ready
            || (crossfading
                && (crossfadeCurrentView.status != SampleFrameViewStatus::ready
                    || crossfadeNextView.status != SampleFrameViewStatus::ready)))
        {
            // The paged policy is bounded silence: advance musical time without waiting.
            ++result.pageMissCount;
            ++result.underrunFrameCount;
            underrunning = true;
            if (currentView.status == SampleFrameViewStatus::pageMissing)
                sample->dataSource->publishPageIntent(
                    frameIndex, SamplePageRequestPriority::imminent, voiceId);
            if (nextView.status == SampleFrameViewStatus::pageMissing)
                sample->dataSource->publishPageIntent(
                    nextFrameIndex, SamplePageRequestPriority::lookAhead, voiceId);
            if (crossfading && crossfadeCurrentView.status == SampleFrameViewStatus::pageMissing)
                sample->dataSource->publishPageIntent(
                    crossfadeFrameIndex, SamplePageRequestPriority::imminent, voiceId);
            if (crossfading && crossfadeNextView.status == SampleFrameViewStatus::pageMissing)
                sample->dataSource->publishPageIntent(
                    crossfadeNextFrameIndex, SamplePageRequestPriority::lookAhead, voiceId);
            advancePitchIncrement();
            envelopeElapsedFrames += 1.0;
            if (loopActive && positionFrames >= static_cast<double>(loopEndFrame))
            {
                const auto loopStart = static_cast<double>(loopStartFrame);
                const auto loopLength = static_cast<double>(loopEndFrame - loopStartFrame);
                positionFrames = loopStart + std::fmod(positionFrames - loopStart, loopLength);
            }
            if (isReleasing() && releaseSamplesRemaining > 0)
            {
                --releaseSamplesRemaining;
                if (releaseSamplesRemaining == 0)
                {
                    finish();
                    break;
                }
            }
            else if (!loopActive && positionFrames >= static_cast<double>(playbackEndFrame))
            {
                finish();
                break;
            }
            continue;
        }
        if (underrunning)
        {
            underrunning = false;
            ++result.recoveryCount;
        }
        if (frameIndex >= nextLookAheadPublicationFrame)
        {
            const auto lookAheadFrame = std::min<std::uint64_t>(
                playbackEndFrame - 1, frameIndex + pageLookAheadFrames);
            sample->dataSource->publishPageIntent(
                lookAheadFrame, SamplePageRequestPriority::lookAhead, voiceId);
            nextLookAheadPublicationFrame = frameIndex > std::numeric_limits<std::uint64_t>::max()
                    - pageIntentCadenceFrames
                ? std::numeric_limits<std::uint64_t>::max()
                : frameIndex + pageIntentCadenceFrames;
        }
        const auto readInterpolated = [&](std::size_t channelIndex) noexcept
        {
            const auto resolvedChannel = std::min(channelIndex, sourceChannelCount - 1);
            const auto current = currentView.channels[resolvedChannel][0];
            const auto next = nextView.channels[resolvedChannel][0];
            return current + (next - current) * fraction;
        };
        const auto readCrossfadeInterpolated = [&](std::size_t channelIndex) noexcept
        {
            const auto resolvedChannel = std::min(channelIndex, sourceChannelCount - 1);
            const auto crossfadeFraction = static_cast<float>(
                crossfadePosition - static_cast<double>(crossfadeFrameIndex));
            const auto current = crossfadeCurrentView.channels[resolvedChannel][0];
            const auto next = crossfadeNextView.channels[resolvedChannel][0];
            return current + (next - current) * crossfadeFraction;
        };
        const auto applyLoopCrossfade = [&](const float tail, const std::size_t channelIndex) noexcept
        {
            if (!crossfading)
                return tail;
            const auto progress = std::clamp(
                (positionFrames - static_cast<double>(crossfadeStartFrame)
                    + (loopCrossfadeFrames == 1 ? 1.0 : 0.0))
                    / static_cast<double>(loopCrossfadeFrames > 1
                        ? loopCrossfadeFrames - 1 : 1),
                0.0, 1.0);
            const auto angle = progress * 0.5 * 3.14159265358979323846;
            const auto tailGain = static_cast<float>(std::cos(angle));
            const auto headGain = static_cast<float>(std::sin(angle));
            return tail * tailGain + readCrossfadeInterpolated(channelIndex) * headGain;
        };

        const auto envelope = static_cast<float>(amplitudeEnvelopeLevel())
            * (isReleasing() ? getReleaseEnvelopeLevel() : 1.0f);
        const auto leftSample = applyLoopCrossfade(readInterpolated(0), 0)
            * baseGain * panGains.left * envelope;
        const auto rightChannel = sourceChannelCount > 1 ? std::size_t { 1 } : std::size_t { 0 };
        const auto rightSource = applyLoopCrossfade(readInterpolated(rightChannel), rightChannel);
        const auto rightSample = rightSource * baseGain * panGains.right * envelope;
        output.channels[0][outputStartFrame + outputFrame] += leftSample;
        if (output.channelCount > 1)
            output.channels[1][outputStartFrame + outputFrame] += rightSample;

        advancePitchIncrement();
        envelopeElapsedFrames += 1.0;
        if (loopActive && positionFrames >= static_cast<double>(loopEndFrame))
        {
            const auto loopStart = static_cast<double>(loopStartFrame);
            const auto loopLength = static_cast<double>(loopEndFrame - loopStartFrame);
            positionFrames = loopStart + std::fmod(positionFrames - loopStart, loopLength);
        }
        ++result.mixedFrameCount;

        if (isReleasing() && releaseSamplesRemaining > 0)
        {
            --releaseSamplesRemaining;
            if (releaseSamplesRemaining == 0)
            {
                finish();
                break;
            }
        }
        else if (!loopActive && positionFrames >= static_cast<double>(playbackEndFrame))
        {
            finish();
            break;
        }
    }

    result.voiceFinished = lifecycleState == SamplerVoiceLifecycleState::finished;
    return result;
}

void SamplerVoice::reset() noexcept
{
    lifecycleState = SamplerVoiceLifecycleState::idle;
    voiceId = 0;
    triggerId = 0;
    activationGeneration = 0;
    routeIndex = 0;
    sourceMidiNote = 0;
    effectiveMidiNote = 0;
    effectiveVelocity = 0;
    renderModel = nullptr;
    route = nullptr;
    sample = nullptr;
    positionFrames = 0.0;
    targetIncrementFrames = 1.0;
    pitchIncrementRampStep = 0.0;
    effectiveTuningCents = 0.0;
    pitchIncrementRampRemaining = 0;
    playbackEndFrame = 0;
    loopStartFrame = 0;
    loopEndFrame = 0;
    loopCrossfadeFrames = 0;
    incrementFrames = 1.0;
    outputSampleRate = 48000.0;
    baseGain = 0.0f;
    unmodulatedGain = 0.0f;
    panGains = {};
    loopActive = false;
    releaseSamplesRemaining = 0;
    releaseSamplesTotal = 0;
    releaseShape = 0.0;
    releaseLevelScale = 1.0f;
    hasReleaseControllerValue = false;
    releaseControllerValue = 0;
    dynamicReleaseUpdateCount = 0;
    repedalCatchCount = 0;
    envelopeHoldSeconds = 0.0;
    envelopeDecaySeconds = 0.0;
    envelopeSustainLevel = 1.0;
    envelopeElapsedFrames = 0.0;
    underrunning = false;
    nextLookAheadPublicationFrame = 0;
}

void SamplerVoice::finish() noexcept
{
    lifecycleState = SamplerVoiceLifecycleState::finished;
    if (playbackEndFrame != 0)
        positionFrames = std::min(positionFrames, static_cast<double>(playbackEndFrame));
    renderModel = nullptr;
    route = nullptr;
    sample = nullptr;
}
} // namespace drs::engine
