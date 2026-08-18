#pragma once

#include <cstdint>
#include <vector>

namespace drs::engine
{
struct WaveformFrameRange
{
    std::uint64_t startFrame = 0;
    std::uint64_t endFrameExclusive = 0;

    bool ordered() const noexcept { return startFrame <= endFrameExclusive; }
    bool empty() const noexcept { return startFrame == endFrameExclusive; }
    std::uint64_t length() const noexcept
    {
        return ordered() ? endFrameExclusive - startFrame : 0;
    }
    bool contains(const std::uint64_t frame) const noexcept
    {
        return frame >= startFrame && frame < endFrameExclusive;
    }
    bool contains(const WaveformFrameRange& other) const noexcept
    {
        return ordered() && other.ordered()
            && other.startFrame >= startFrame
            && other.endFrameExclusive <= endFrameExclusive;
    }
};

struct WaveformViewport
{
    WaveformFrameRange frames;
    double widthPixels = 0.0;
};

WaveformFrameRange normalizeWaveformFrameRange(WaveformFrameRange requested,
                                                WaveformFrameRange legalBounds,
                                                std::uint64_t minimumLength) noexcept;
WaveformFrameRange normalizePlaybackRegion(WaveformFrameRange requested,
                                           std::uint64_t sourceFrameCount) noexcept;
WaveformFrameRange normalizeLoopRegion(WaveformFrameRange requested,
                                       WaveformFrameRange playbackRegion) noexcept;
WaveformFrameRange normalizeSelectionRegion(WaveformFrameRange requested,
                                            std::uint64_t sourceFrameCount) noexcept;

double waveformFrameToPixel(std::uint64_t frame,
                            const WaveformViewport& viewport) noexcept;
std::uint64_t waveformPixelToFrame(double pixel,
                                   const WaveformViewport& viewport) noexcept;
WaveformFrameRange zoomWaveformViewport(WaveformFrameRange current,
                                        std::uint64_t anchorFrame,
                                        double scale,
                                        std::uint64_t sourceFrameCount,
                                        std::uint64_t minimumLength = 32) noexcept;
WaveformFrameRange panWaveformViewport(WaveformFrameRange current,
                                       std::int64_t deltaFrames,
                                       std::uint64_t sourceFrameCount) noexcept;

enum class WaveformRegionBoundary : std::uint8_t
{
    playbackStart = 0,
    playbackEnd,
    loopStart,
    loopEnd
};

struct WaveformEditableRegions
{
    WaveformFrameRange playback;
    WaveformFrameRange loop;
    bool loopActive = false;
};

WaveformEditableRegions normalizeBoundaryDrag(WaveformEditableRegions regions,
                                               WaveformRegionBoundary boundary,
                                               std::uint64_t candidateFrame,
                                               std::uint64_t sourceFrameCount) noexcept;

WaveformEditableRegions movePlaybackRegion(WaveformEditableRegions regions,
                                           std::int64_t deltaFrames,
                                           std::uint64_t sourceFrameCount) noexcept;

struct WaveformSnapDecision
{
    std::uint64_t requestedFrame = 0;
    std::uint64_t resolvedFrame = 0;
    bool applied = false;
};

WaveformSnapDecision chooseWaveformSnapCandidate(
    std::uint64_t requestedFrame,
    const std::vector<std::uint64_t>& candidateFrames,
    WaveformFrameRange legalBounds,
    std::uint64_t maximumDistance,
    bool bypassSnap = false) noexcept;

enum class WaveformAuditionMode : std::uint8_t
{
    playbackRegion = 0,
    loopRegion,
    selection
};

struct WaveformAuditionRegion
{
    WaveformFrameRange playback;
    WaveformFrameRange loop;
    bool loopActive = false;
    bool valid = false;
};

WaveformAuditionRegion resolveWaveformAuditionRegion(
    WaveformAuditionMode mode,
    WaveformEditableRegions regions,
    WaveformFrameRange selection,
    std::uint64_t sourceFrameCount) noexcept;

enum class WaveformEditGestureState : std::uint8_t
{
    idle = 0,
    active,
    committed,
    canceled
};

enum class WaveformEditGestureEvent : std::uint8_t
{
    begin = 0,
    update,
    commit,
    cancel,
    lostCapture,
    zoneChanged,
    sourceChanged,
    reset
};

struct WaveformEditGestureTransition
{
    bool accepted = false;
    bool shouldCommit = false;
    WaveformEditGestureState state = WaveformEditGestureState::idle;
};

WaveformEditGestureTransition transitionWaveformEditGesture(
    WaveformEditGestureState state,
    WaveformEditGestureEvent event) noexcept;
} // namespace drs::engine
