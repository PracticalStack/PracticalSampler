#include "drs/engine/WaveformRegionPolicy.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto zeroLength = normalizePlaybackRegion({ 10, 20 }, 0);
        require(zeroLength.startFrame == 0 && zeroLength.endFrameExclusive == 0,
                "A zero-length source must normalize to an empty region without underflow.");

        const auto oneFrame = normalizePlaybackRegion({ 1, 0 }, 1);
        require(oneFrame.startFrame == 0 && oneFrame.endFrameExclusive == 1,
                "A one-frame source must retain its only legal half-open playback region.");

        const auto reversed = normalizePlaybackRegion({ 90, 10 }, 100);
        require(reversed.startFrame == 10 && reversed.endFrameExclusive == 90,
                "General range normalization should deterministically order crossed inputs.");

        const auto clipped = normalizePlaybackRegion({ 90, 200 }, 100);
        require(clipped.startFrame == 90 && clipped.endFrameExclusive == 100,
                "Playback normalization must clamp to the source frame count.");

        const auto oneFrameAtEnd = normalizePlaybackRegion({ 100, 100 }, 100);
        require(oneFrameAtEnd.startFrame == 99 && oneFrameAtEnd.endFrameExclusive == 100,
                "A collapsed playback range at source end should move left to retain one frame.");

        const WaveformFrameRange playback { 20, 80 };
        const auto containedLoop = normalizeLoopRegion({ 10, 90 }, playback);
        require(containedLoop.startFrame == 20 && containedLoop.endFrameExclusive == 80,
                "Loop normalization must keep the loop inside the playback region.");

        const auto emptySelection = normalizeSelectionRegion({ 42, 42 }, 100);
        require(emptySelection.empty() && emptySelection.startFrame == 42,
                "Temporary selection may be empty without creating an authored edit.");

        const WaveformViewport ordinaryViewport { { 100, 200 }, 400.0 };
        require(std::abs(waveformFrameToPixel(150, ordinaryViewport) - 200.0) < 0.000001,
                "Frame-to-pixel conversion should map the viewport midpoint exactly.");
        require(waveformPixelToFrame(200.0, ordinaryViewport) == 150
                    && waveformPixelToFrame(-10.0, ordinaryViewport) == 100
                    && waveformPixelToFrame(500.0, ordinaryViewport) == 200,
                "Pixel-to-frame conversion should be stable and clamp to half-open viewport boundaries.");

        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        const WaveformViewport largeViewport { { maximum - 1000, maximum }, 1000.0 };
        require(std::abs(waveformFrameToPixel(maximum - 500, largeViewport) - 500.0) < 0.000001,
                "Frame-to-pixel conversion must not overflow near the 64-bit frame limit.");
        require(waveformPixelToFrame(750.0, largeViewport) == maximum - 250,
                "Pixel-to-frame conversion must preserve deep 64-bit source positions.");

        const auto zoomed = zoomWaveformViewport({ 0, 1000 }, 250, 0.5, 1000, 32);
        require(zoomed.startFrame == 125 && zoomed.endFrameExclusive == 625,
                "Pointer-centered zoom should preserve the anchor's relative viewport position.");
        const auto pannedLeft = panWaveformViewport(zoomed, -500, 1000);
        const auto pannedRight = panWaveformViewport(zoomed, 900, 1000);
        require(pannedLeft.startFrame == 0 && pannedLeft.endFrameExclusive == 500
                    && pannedRight.startFrame == 500 && pannedRight.endFrameExclusive == 1000,
                "Viewport panning should preserve span while clamping at both source edges.");

        WaveformEditableRegions regions { { 10, 90 }, { 30, 70 }, true };
        regions = normalizeBoundaryDrag(regions,
                                        WaveformRegionBoundary::playbackStart,
                                        50,
                                        100);
        require(regions.playback.startFrame == 30,
                "Playback start must stop at an active loop start instead of excluding the loop.");

        regions = normalizeBoundaryDrag(regions,
                                        WaveformRegionBoundary::playbackEnd,
                                        40,
                                        100);
        require(regions.playback.endFrameExclusive == 70,
                "Playback end must stop at an active loop end instead of excluding the loop.");

        regions = normalizeBoundaryDrag(regions,
                                        WaveformRegionBoundary::loopStart,
                                        90,
                                        100);
        require(regions.loop.startFrame == 69,
                "Loop-start handles must not cross the exclusive loop end.");

        regions = normalizeBoundaryDrag(regions,
                                        WaveformRegionBoundary::loopEnd,
                                        0,
                                        100);
        require(regions.loop.endFrameExclusive == 70,
                "Loop-end handles must preserve a one-frame minimum loop.");
        require(regions.playback.contains(regions.loop),
                "Every accepted boundary drag must preserve loop containment.");

        const auto idleUpdate = transitionWaveformEditGesture(
            WaveformEditGestureState::idle,
            WaveformEditGestureEvent::update);
        require(!idleUpdate.accepted && idleUpdate.state == WaveformEditGestureState::idle,
                "Pointer updates without an active gesture must be ignored.");

        const auto begun = transitionWaveformEditGesture(
            WaveformEditGestureState::idle,
            WaveformEditGestureEvent::begin);
        const auto updated = transitionWaveformEditGesture(
            begun.state,
            WaveformEditGestureEvent::update);
        const auto committed = transitionWaveformEditGesture(
            updated.state,
            WaveformEditGestureEvent::commit);
        require(begun.accepted && updated.accepted && committed.accepted
                    && committed.shouldCommit
                    && committed.state == WaveformEditGestureState::committed,
                "A completed gesture should request exactly one commit at its terminal transition.");

        for (const auto cancelEvent : { WaveformEditGestureEvent::cancel,
                                        WaveformEditGestureEvent::lostCapture,
                                        WaveformEditGestureEvent::zoneChanged,
                                        WaveformEditGestureEvent::sourceChanged })
        {
            const auto canceled = transitionWaveformEditGesture(
                WaveformEditGestureState::active,
                cancelEvent);
            require(canceled.accepted && !canceled.shouldCommit
                        && canceled.state == WaveformEditGestureState::canceled,
                    "Cancel, lost capture, zone change, and source change must terminate without commit.");
        }

        const auto reset = transitionWaveformEditGesture(
            WaveformEditGestureState::committed,
            WaveformEditGestureEvent::reset);
        require(reset.accepted && reset.state == WaveformEditGestureState::idle,
                "Terminal gesture states should reset explicitly to idle.");

        std::cout << "Waveform region policy tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Waveform region policy tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
