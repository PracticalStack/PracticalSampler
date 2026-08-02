#include "drs/engine/SamplerVoice.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
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
    if (selectedSample.decodedSampleData == nullptr
        || selectedSample.frameCount == 0
        || selectedRoute.sampleStartFrame >= selectedSample.frameCount
        || !std::isfinite(selectedSample.sampleRate) || selectedSample.sampleRate <= 0.0)
    {
        return false;
    }

    const auto pitchRatio = std::pow(2.0,
                                     static_cast<double>(request.effectiveMidiNote - selectedRoute.rootKey) / 12.0);
    const auto increment = pitchRatio * (selectedSample.sampleRate / request.outputSampleRate);
    const auto gain = 0.25
        * (static_cast<double>(request.effectiveVelocity) / 127.0)
        * request.routeGainMultiplier
        * std::pow(10.0, selectedRoute.gainDb / 20.0);
    if (!std::isfinite(increment) || increment <= 0.0 || !std::isfinite(gain))
        return false;

    lifecycleState = SamplerVoiceLifecycleState::active;
    voiceId = request.voiceId;
    activationGeneration = request.activationGeneration;
    routeIndex = request.routeIndex;
    sourceMidiNote = request.sourceMidiNote;
    effectiveMidiNote = request.effectiveMidiNote;
    effectiveVelocity = request.effectiveVelocity;
    renderModel = &model;
    route = &selectedRoute;
    sample = &selectedSample;
    positionFrames = static_cast<double>(selectedRoute.sampleStartFrame);
    incrementFrames = increment;
    outputSampleRate = request.outputSampleRate;
    baseGain = static_cast<float>(gain);
    panGains = computeSamplerPanGains(selectedRoute.pan);
    loopActive = selectedRoute.loopEnabled
        && selectedRoute.loopStartFrame < selectedRoute.loopEndFrame
        && selectedRoute.loopEndFrame <= selectedSample.frameCount
        && selectedRoute.sampleStartFrame < selectedRoute.loopEndFrame;
    return true;
}

bool SamplerVoice::beginRelease(const double overrideReleaseSeconds) noexcept
{
    if (lifecycleState == SamplerVoiceLifecycleState::releasing)
        return false;
    if (lifecycleState != SamplerVoiceLifecycleState::active)
        return false;

    lifecycleState = SamplerVoiceLifecycleState::releasing;
    const auto releaseSeconds = overrideReleaseSeconds > 0.0
        ? overrideReleaseSeconds : (route != nullptr ? route->releaseSeconds : 0.0);
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

    const auto& decodedChannels = sample->decodedSampleData->normalizedChannels;
    const auto sourceChannelCount = decodedChannels.size();
    for (std::uint32_t outputFrame = 0; outputFrame < frameCount; ++outputFrame)
    {
        if (positionFrames >= static_cast<double>(sample->frameCount))
        {
            finish();
            break;
        }

        const auto frameIndex = static_cast<std::size_t>(positionFrames);
        auto nextFrameIndex = std::min(frameIndex + 1,
                                       static_cast<std::size_t>(sample->frameCount - 1));
        if (loopActive
            && frameIndex < route->loopEndFrame
            && nextFrameIndex >= route->loopEndFrame)
        {
            nextFrameIndex = static_cast<std::size_t>(route->loopStartFrame);
        }
        const auto fraction = static_cast<float>(positionFrames - static_cast<double>(frameIndex));
        const auto readInterpolated = [&](std::size_t channelIndex) noexcept
        {
            const auto resolvedChannel = std::min(channelIndex, sourceChannelCount - 1);
            const auto& channel = decodedChannels[resolvedChannel];
            const auto current = channel[frameIndex];
            const auto next = channel[nextFrameIndex];
            return current + (next - current) * fraction;
        };

        const auto envelope = isReleasing() && releaseSamplesTotal > 0
            ? static_cast<float>(releaseSamplesRemaining) / static_cast<float>(releaseSamplesTotal)
            : 1.0f;
        const auto leftSample = readInterpolated(0) * baseGain * panGains.left * envelope;
        const auto rightSource = sourceChannelCount > 1 ? readInterpolated(1) : readInterpolated(0);
        const auto rightSample = rightSource * baseGain * panGains.right * envelope;
        output.channels[0][outputStartFrame + outputFrame] += leftSample;
        if (output.channelCount > 1)
            output.channels[1][outputStartFrame + outputFrame] += rightSample;

        positionFrames += incrementFrames;
        if (loopActive && positionFrames >= static_cast<double>(route->loopEndFrame))
        {
            const auto loopStart = static_cast<double>(route->loopStartFrame);
            const auto loopLength = static_cast<double>(route->loopEndFrame - route->loopStartFrame);
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
        else if (!loopActive && positionFrames >= static_cast<double>(sample->frameCount))
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
    activationGeneration = 0;
    routeIndex = 0;
    sourceMidiNote = 0;
    effectiveMidiNote = 0;
    effectiveVelocity = 0;
    renderModel = nullptr;
    route = nullptr;
    sample = nullptr;
    positionFrames = 0.0;
    incrementFrames = 1.0;
    outputSampleRate = 48000.0;
    baseGain = 0.0f;
    panGains = {};
    loopActive = false;
    releaseSamplesRemaining = 0;
    releaseSamplesTotal = 0;
}

void SamplerVoice::finish() noexcept
{
    lifecycleState = SamplerVoiceLifecycleState::finished;
    if (sample != nullptr)
        positionFrames = std::min(positionFrames, static_cast<double>(sample->frameCount));
    renderModel = nullptr;
    route = nullptr;
    sample = nullptr;
}
} // namespace drs::engine
