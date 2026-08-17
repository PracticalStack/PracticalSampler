#include "drs/engine/WaveformRegionPolicy.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
namespace
{
std::uint64_t clampFrame(const std::uint64_t frame,
                         const std::uint64_t minimum,
                         const std::uint64_t maximum) noexcept
{
    return std::min(std::max(frame, minimum), maximum);
}
} // namespace

WaveformFrameRange normalizeWaveformFrameRange(WaveformFrameRange requested,
                                                WaveformFrameRange legalBounds,
                                                const std::uint64_t minimumLength) noexcept
{
    if (!legalBounds.ordered())
        std::swap(legalBounds.startFrame, legalBounds.endFrameExclusive);

    requested.startFrame = clampFrame(requested.startFrame,
                                      legalBounds.startFrame,
                                      legalBounds.endFrameExclusive);
    requested.endFrameExclusive = clampFrame(requested.endFrameExclusive,
                                             legalBounds.startFrame,
                                             legalBounds.endFrameExclusive);
    if (!requested.ordered())
        std::swap(requested.startFrame, requested.endFrameExclusive);

    const auto legalLength = legalBounds.length();
    const auto requiredLength = std::min(minimumLength, legalLength);
    if (requested.length() >= requiredLength)
        return requested;

    const auto availableAfterStart = legalBounds.endFrameExclusive - requested.startFrame;
    if (availableAfterStart >= requiredLength)
    {
        requested.endFrameExclusive = requested.startFrame + requiredLength;
        return requested;
    }

    requested.endFrameExclusive = legalBounds.endFrameExclusive;
    requested.startFrame = requested.endFrameExclusive - requiredLength;
    return requested;
}

WaveformFrameRange normalizePlaybackRegion(const WaveformFrameRange requested,
                                           const std::uint64_t sourceFrameCount) noexcept
{
    return normalizeWaveformFrameRange(requested,
                                       { 0, sourceFrameCount },
                                       sourceFrameCount > 0 ? 1 : 0);
}

WaveformFrameRange normalizeLoopRegion(const WaveformFrameRange requested,
                                       const WaveformFrameRange playbackRegion) noexcept
{
    return normalizeWaveformFrameRange(requested,
                                       playbackRegion,
                                       playbackRegion.empty() ? 0 : 1);
}

WaveformFrameRange normalizeSelectionRegion(const WaveformFrameRange requested,
                                            const std::uint64_t sourceFrameCount) noexcept
{
    return normalizeWaveformFrameRange(requested, { 0, sourceFrameCount }, 0);
}

double waveformFrameToPixel(const std::uint64_t frame,
                            const WaveformViewport& viewport) noexcept
{
    if (!viewport.frames.ordered() || viewport.frames.empty()
        || !std::isfinite(viewport.widthPixels) || viewport.widthPixels <= 0.0)
        return 0.0;

    const auto clamped = clampFrame(frame,
                                    viewport.frames.startFrame,
                                    viewport.frames.endFrameExclusive);
    const auto offset = static_cast<long double>(clamped - viewport.frames.startFrame);
    const auto length = static_cast<long double>(viewport.frames.length());
    const auto width = static_cast<long double>(viewport.widthPixels);
    return static_cast<double>((offset / length) * width);
}

std::uint64_t waveformPixelToFrame(const double pixel,
                                   const WaveformViewport& viewport) noexcept
{
    if (!viewport.frames.ordered() || viewport.frames.empty()
        || !std::isfinite(viewport.widthPixels) || viewport.widthPixels <= 0.0
        || !std::isfinite(pixel))
        return viewport.frames.startFrame;

    const auto clampedPixel = std::min(std::max(pixel, 0.0), viewport.widthPixels);
    if (clampedPixel >= viewport.widthPixels)
        return viewport.frames.endFrameExclusive;

    const auto ratio = static_cast<long double>(clampedPixel)
        / static_cast<long double>(viewport.widthPixels);
    const auto offset = static_cast<std::uint64_t>(
        ratio * static_cast<long double>(viewport.frames.length()));
    return viewport.frames.startFrame + std::min(offset, viewport.frames.length());
}

WaveformFrameRange zoomWaveformViewport(WaveformFrameRange current,
                                        const std::uint64_t anchorFrame,
                                        const double scale,
                                        const std::uint64_t sourceFrameCount,
                                        const std::uint64_t minimumLength) noexcept
{
    current = normalizeWaveformFrameRange(current,
                                          { 0, sourceFrameCount },
                                          std::min(minimumLength, sourceFrameCount));
    if (sourceFrameCount == 0 || !std::isfinite(scale) || scale <= 0.0)
        return current;

    const auto oldLength = current.length();
    const auto targetLength = std::min<std::uint64_t>(
        sourceFrameCount,
        std::max<std::uint64_t>(std::min(minimumLength, sourceFrameCount),
                                static_cast<std::uint64_t>(std::llround(
                                    static_cast<long double>(oldLength) * scale))));
    const auto clampedAnchor = clampFrame(anchorFrame, current.startFrame, current.endFrameExclusive);
    const auto anchorRatio = oldLength == 0
        ? 0.5L
        : static_cast<long double>(clampedAnchor - current.startFrame)
            / static_cast<long double>(oldLength);
    const auto beforeAnchor = static_cast<std::uint64_t>(
        std::llround(anchorRatio * static_cast<long double>(targetLength)));
    const auto proposedStart = clampedAnchor > beforeAnchor ? clampedAnchor - beforeAnchor : 0;
    const auto start = std::min(proposedStart, sourceFrameCount - targetLength);
    return normalizeWaveformFrameRange({ start, start + targetLength },
                                       { 0, sourceFrameCount },
                                       targetLength);
}

WaveformFrameRange panWaveformViewport(WaveformFrameRange current,
                                       const std::int64_t deltaFrames,
                                       const std::uint64_t sourceFrameCount) noexcept
{
    current = normalizeWaveformFrameRange(current, { 0, sourceFrameCount }, 0);
    const auto length = current.length();
    if (length >= sourceFrameCount || deltaFrames == 0)
        return current;

    std::uint64_t start = current.startFrame;
    if (deltaFrames < 0)
    {
        const auto magnitude = static_cast<std::uint64_t>(-(deltaFrames + 1)) + 1;
        start = magnitude >= start ? 0 : start - magnitude;
    }
    else
    {
        const auto maximumStart = sourceFrameCount - length;
        const auto magnitude = static_cast<std::uint64_t>(deltaFrames);
        start = magnitude >= maximumStart - std::min(start, maximumStart)
            ? maximumStart
            : start + magnitude;
    }
    return { start, start + length };
}

WaveformEditableRegions normalizeBoundaryDrag(WaveformEditableRegions regions,
                                               const WaveformRegionBoundary boundary,
                                               const std::uint64_t candidateFrame,
                                               const std::uint64_t sourceFrameCount) noexcept
{
    regions.playback = normalizePlaybackRegion(regions.playback, sourceFrameCount);
    regions.loop = normalizeLoopRegion(regions.loop, regions.playback);
    if (sourceFrameCount == 0)
    {
        regions.loopActive = false;
        return regions;
    }

    switch (boundary)
    {
        case WaveformRegionBoundary::playbackStart:
        {
            auto maximum = regions.playback.endFrameExclusive - 1;
            if (regions.loopActive)
                maximum = std::min(maximum, regions.loop.startFrame);
            regions.playback.startFrame = clampFrame(candidateFrame, 0, maximum);
            break;
        }
        case WaveformRegionBoundary::playbackEnd:
        {
            auto minimum = regions.playback.startFrame + 1;
            if (regions.loopActive)
                minimum = std::max(minimum, regions.loop.endFrameExclusive);
            regions.playback.endFrameExclusive = clampFrame(candidateFrame,
                                                             minimum,
                                                             sourceFrameCount);
            break;
        }
        case WaveformRegionBoundary::loopStart:
        {
            if (!regions.loopActive)
                break;
            const auto maximum = regions.loop.endFrameExclusive - 1;
            regions.loop.startFrame = clampFrame(candidateFrame,
                                                  regions.playback.startFrame,
                                                  maximum);
            break;
        }
        case WaveformRegionBoundary::loopEnd:
        {
            if (!regions.loopActive)
                break;
            const auto minimum = regions.loop.startFrame + 1;
            regions.loop.endFrameExclusive = clampFrame(candidateFrame,
                                                         minimum,
                                                         regions.playback.endFrameExclusive);
            break;
        }
    }

    return regions;
}

WaveformEditGestureTransition transitionWaveformEditGesture(
    const WaveformEditGestureState state,
    const WaveformEditGestureEvent event) noexcept
{
    using State = WaveformEditGestureState;
    using Event = WaveformEditGestureEvent;

    if (event == Event::reset)
        return { true, false, State::idle };

    switch (state)
    {
        case State::idle:
            if (event == Event::begin)
                return { true, false, State::active };
            break;
        case State::active:
            if (event == Event::update)
                return { true, false, State::active };
            if (event == Event::commit)
                return { true, true, State::committed };
            if (event == Event::cancel || event == Event::lostCapture
                || event == Event::zoneChanged || event == Event::sourceChanged)
                return { true, false, State::canceled };
            break;
        case State::committed:
        case State::canceled:
            if (event == Event::begin)
                return { true, false, State::active };
            break;
    }

    return { false, false, state };
}
} // namespace drs::engine
