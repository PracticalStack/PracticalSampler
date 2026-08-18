#include "shared/authoring/WaveformDetailView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::app::authoring
{
namespace
{
const auto waveformGrid = visual::mapSurface;
const auto waveformMuted = visual::textMuted;
const auto waveformSelected = visual::modulation;
const auto waveformAccent = visual::selection;
} // namespace

WaveformDetailView::WaveformDetailView()
{
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    startTimerHz(30);
}

void WaveformDetailView::setPreview(AuthoringWaveformPreview nextPreview)
{
    const auto sourceChanged = preview.sourceIdentity != nextPreview.sourceIdentity
        || preview.frameCount != nextPreview.frameCount;
    if (sourceChanged && gesture != Gesture::none)
        cancelRegionGesture();
    if (!sourceChanged && preview.selectionActive)
    {
        nextPreview.selectionActive = true;
        nextPreview.selectionStartFrame = preview.selectionStartFrame;
        nextPreview.selectionEndFrameExclusive = preview.selectionEndFrameExclusive;
    }
    preview = std::move(nextPreview);
    if (sourceChanged || viewportFrames.empty())
        fitToSource(false);
    repaint();
}

void WaveformDetailView::setDetailRequestCallback(DetailRequestCallback callback)
{
    detailRequestCallback = std::move(callback);
}

void WaveformDetailView::setLoopRegionCommitCallback(LoopRegionCommitCallback callback)
{
    loopRegionCommitCallback = std::move(callback);
}

void WaveformDetailView::setPlaybackRegionCommitCallback(PlaybackRegionCommitCallback callback)
{
    playbackRegionCommitCallback = std::move(callback);
}

void WaveformDetailView::setZeroCrossingSnapEnabled(const bool enabled)
{
    zeroCrossingSnapEnabled = enabled;
    if (!enabled)
    {
        snapService.cancel("Zero-crossing snapping disabled");
        pendingSnapGeneration = 0;
        snapApplied = false;
        snapStatus = "Frame snap";
    }
    else
    {
        snapStatus = "Zero-crossing snap ready · hold Alt to bypass";
    }
    repaint();
}

drs::engine::WaveformFrameRange WaveformDetailView::getSelectionFrames() const noexcept
{
    return preview.selectionActive
        ? drs::engine::normalizeSelectionRegion(
            { preview.selectionStartFrame, preview.selectionEndFrameExclusive }, preview.frameCount)
        : drs::engine::WaveformFrameRange {};
}

void WaveformDetailView::clearSelection()
{
    preview.selectionActive = false;
    preview.selectionStartFrame = 0;
    preview.selectionEndFrameExclusive = 0;
    repaint();
}

void WaveformDetailView::setSelectionFrames(const drs::engine::WaveformFrameRange selection)
{
    const auto normalized = drs::engine::normalizeSelectionRegion(selection, preview.frameCount);
    preview.selectionActive = !normalized.empty();
    preview.selectionStartFrame = normalized.startFrame;
    preview.selectionEndFrameExclusive = normalized.endFrameExclusive;
    repaint();
}

drs::engine::WaveformEditableRegions WaveformDetailView::currentRegions() const noexcept
{
    auto playbackEnd = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    if (preview.playbackStartFrame >= playbackEnd)
        playbackEnd = preview.frameCount;
    return { { preview.playbackStartFrame, playbackEnd },
             { preview.loopStartFrame, preview.loopEndFrame },
             preview.loopEnabled };
}

juce::Rectangle<float> WaveformDetailView::getCanvasBounds() const
{
    return getLocalBounds().toFloat().reduced(12.0f);
}

void WaveformDetailView::fitToSource(const bool requestDetail)
{
    viewportFrames = { 0, preview.frameCount };
    if (requestDetail)
        publishDetailRequest();
    repaint();
}

void WaveformDetailView::publishDetailRequest()
{
    if (!detailRequestCallback || preview.frameCount == 0 || viewportFrames.empty()
        || viewportFrames.length() >= preview.frameCount)
    {
        return;
    }

    const auto pointCount = static_cast<std::size_t>(juce::jlimit(
        256, 4096, std::max(1, getCanvasBounds().getWidth() > 0
            ? static_cast<int>(std::ceil(getCanvasBounds().getWidth() * 2.0f))
            : 256)));
    detailRequestCallback(viewportFrames.startFrame,
                          viewportFrames.endFrameExclusive,
                          pointCount);
}

void WaveformDetailView::panByFrames(const std::int64_t frames, const bool publishRequest)
{
    viewportFrames = drs::engine::panWaveformViewport(viewportFrames, frames, preview.frameCount);
    if (publishRequest)
        publishDetailRequest();
    repaint();
}

void WaveformDetailView::drawPeaks(
    juce::Graphics& g,
    const std::vector<AuthoringWaveformPreviewPoint>& points,
    const drs::engine::WaveformFrameRange coverage,
    const juce::Colour colour,
    const float thickness) const
{
    if (points.empty() || coverage.empty() || viewportFrames.empty())
        return;

    const auto inner = getCanvasBounds();
    const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
    juce::Path waveformPath;
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const auto ratio = (static_cast<long double>(index) + 0.5L)
            / static_cast<long double>(points.size());
        const auto frame = coverage.startFrame + static_cast<std::uint64_t>(
            ratio * static_cast<long double>(coverage.length()));
        if (frame < viewportFrames.startFrame || frame > viewportFrames.endFrameExclusive)
            continue;

        const auto x = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(frame, viewport));
        const auto minY = juce::jmap(points[index].minValue,
                                     -1.0f, 1.0f, inner.getBottom(), inner.getY());
        const auto maxY = juce::jmap(points[index].maxValue,
                                     -1.0f, 1.0f, inner.getBottom(), inner.getY());
        waveformPath.startNewSubPath(x, minY);
        waveformPath.lineTo(x, maxY);
    }
    g.setColour(colour);
    g.strokePath(waveformPath, juce::PathStrokeType(thickness));
}

void WaveformDetailView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(waveformGrid);
    g.fillRoundedRectangle(bounds, visual::panelRadius);
    g.setColour(visual::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), visual::panelRadius, visual::borderWidth);

    const auto inner = getCanvasBounds();
    g.setColour(visual::text.withAlpha(0.16f));
    g.drawHorizontalLine(static_cast<int>(inner.getCentreY()), inner.getX(), inner.getRight());

    if (!viewportFrames.empty())
    {
        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        for (int division = 1; division < 8; ++division)
        {
            const auto x = inner.getX() + inner.getWidth() * static_cast<float>(division) / 8.0f;
            g.setColour(visual::text.withAlpha(division == 4 ? 0.12f : 0.06f));
            g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
        }

        if (preview.selectionActive)
        {
            const auto selection = drs::engine::normalizeSelectionRegion(
                { preview.selectionStartFrame, preview.selectionEndFrameExclusive },
                preview.frameCount);
            const auto x1 = inner.getX() + static_cast<float>(
                drs::engine::waveformFrameToPixel(selection.startFrame, viewport));
            const auto x2 = inner.getX() + static_cast<float>(
                drs::engine::waveformFrameToPixel(selection.endFrameExclusive, viewport));
            g.setColour(waveformAccent.withAlpha(0.12f));
            g.fillRect(juce::Rectangle<float>(x1, inner.getY(), x2 - x1, inner.getHeight()));
        }
    }

    const auto drawPlaybackMarkers = [&]
    {
        if (preview.frameCount == 0 || viewportFrames.empty())
            return;
        const auto regions = currentRegions();
        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        const auto startX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(regions.playback.startFrame, viewport));
        const auto endX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(regions.playback.endFrameExclusive, viewport));
        const auto visibleStart = juce::jlimit(inner.getX(), inner.getRight(), startX);
        const auto visibleEnd = juce::jlimit(inner.getX(), inner.getRight(), endX);
        g.setColour(waveformGrid.withAlpha(0.72f));
        if (visibleStart > inner.getX())
            g.fillRect(juce::Rectangle<float>(inner.getX(), inner.getY(),
                                              visibleStart - inner.getX(), inner.getHeight()));
        if (visibleEnd < inner.getRight())
            g.fillRect(juce::Rectangle<float>(visibleEnd, inner.getY(),
                                              inner.getRight() - visibleEnd, inner.getHeight()));

        g.setColour(waveformSelected);
        g.drawVerticalLine(static_cast<int>(startX), inner.getY(), inner.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), inner.getY(), inner.getBottom());
        g.fillRoundedRectangle(startX - 5.0f, inner.getY(), 10.0f, 18.0f, 2.0f);
        g.fillRoundedRectangle(endX - 5.0f, inner.getY(), 10.0f, 18.0f, 2.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawFittedText("START", juce::Rectangle<int>(static_cast<int>(startX + 7.0f),
                                                        static_cast<int>(inner.getY() + 2.0f), 42, 14),
                         juce::Justification::centredLeft, 1);
        g.drawFittedText("END", juce::Rectangle<int>(static_cast<int>(endX - 45.0f),
                                                      static_cast<int>(inner.getY() + 2.0f), 38, 14),
                         juce::Justification::centredRight, 1);
    };

    const auto drawLoopMarkers = [&]
    {
        if (!preview.loopEnabled || preview.frameCount == 0)
            return;

        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        const auto startX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(preview.loopStartFrame, viewport));
        const auto endX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(preview.loopEndFrame, viewport));
        const auto visibleLeft = juce::jlimit(inner.getX(), inner.getRight(), startX);
        const auto visibleRight = juce::jlimit(inner.getX(), inner.getRight(), endX);
        if (visibleRight > visibleLeft)
        {
            g.setColour(waveformAccent.withAlpha(0.10f));
            g.fillRect(juce::Rectangle<float>(visibleLeft, inner.getY(),
                                              visibleRight - visibleLeft, inner.getHeight()));
        }
        g.setColour(waveformAccent);
        g.drawVerticalLine(static_cast<int>(startX), inner.getY(), inner.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), inner.getY(), inner.getBottom());
        g.fillRect(startX - 3.0f, inner.getY(), 6.0f, 8.0f);
        g.fillRect(endX - 3.0f, inner.getBottom() - 8.0f, 6.0f, 8.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawFittedText("LOOP IN", juce::Rectangle<int>(static_cast<int>(startX + 5.0f),
                                                         static_cast<int>(inner.getY()), 54, 14),
                         juce::Justification::centredLeft, 1);
        g.drawFittedText("LOOP OUT", juce::Rectangle<int>(static_cast<int>(endX - 59.0f),
                                                          static_cast<int>(inner.getBottom() - 14.0f), 54, 14),
                         juce::Justification::centredRight, 1);
    };

    if (!preview.available || preview.points.empty())
    {
        drawPlaybackMarkers();
        drawLoopMarkers();
        g.setColour(waveformMuted);
        g.drawFittedText(preview.state.empty() ? "Waveform unavailable" : juce::String::fromUTF8(preview.state.c_str()),
                         getLocalBounds().reduced(12),
                         juce::Justification::centred,
                         2);
        return;
    }

    const auto hasCoveringDetail = !preview.detailPoints.empty()
        && preview.detailStartFrame <= viewportFrames.startFrame
        && preview.detailEndFrameExclusive >= viewportFrames.endFrameExclusive;
    if (hasCoveringDetail)
    {
        drawPeaks(g, preview.detailPoints,
                  { preview.detailStartFrame, preview.detailEndFrameExclusive },
                  waveformSelected, 1.3f);
    }
    else
    {
        // The overview remains source-aligned. Zooming therefore reveals its
        // actual sparse resolution instead of stretching 192 points as detail.
        drawPeaks(g, preview.points, { 0, preview.frameCount },
                  waveformSelected.withAlpha(0.52f), 1.0f);
    }
    drawPlaybackMarkers();
    drawLoopMarkers();

    if (preview.playheadVisible && !viewportFrames.empty())
    {
        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        const auto x = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(preview.playheadFrame, viewport));
        g.setColour(visual::success);
        g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
    }

    if (!preview.state.empty() && preview.state != "Ready")
    {
        g.setColour(waveformMuted);
        g.drawFittedText(juce::String::fromUTF8(preview.state.c_str()),
                         getLocalBounds().reduced(12).removeFromTop(20),
                         juce::Justification::centredLeft,
                         1);
    }

    if (zeroCrossingSnapEnabled && !snapStatus.empty())
    {
        g.setColour(snapApplied ? visual::success : waveformMuted);
        g.setFont(juce::FontOptions(10.0f));
        g.drawFittedText(juce::String::fromUTF8(snapStatus.c_str()),
                         getLocalBounds().reduced(12).removeFromBottom(16),
                         juce::Justification::centredRight, 1);
    }
}

std::uint64_t WaveformDetailView::frameAtX(const float x) const noexcept
{
    const auto inner = getCanvasBounds();
    return drs::engine::waveformPixelToFrame(
        static_cast<double>(x - inner.getX()),
        { viewportFrames, inner.getWidth() });
}

void WaveformDetailView::cancelRegionGesture()
{
    if (gesture == Gesture::loopStart || gesture == Gesture::loopEnd
        || gesture == Gesture::playbackStart || gesture == Gesture::playbackEnd
        || gesture == Gesture::playbackMove)
    {
        preview.playbackStartFrame = originalPlaybackStartFrame;
        preview.playbackEndFrameExclusive = originalPlaybackEndFrame;
        preview.loopStartFrame = originalLoopStartFrame;
        preview.loopEndFrame = originalLoopEndFrame;
    }
    snapService.cancel("Waveform boundary gesture canceled");
    pendingSnapGeneration = 0;
    snapApplied = false;
    gesture = Gesture::none;
    dragging = false;
    repaint();
}

void WaveformDetailView::applyBoundaryCandidate(const std::uint64_t candidateFrame,
                                                const bool snapped)
{
    auto boundary = drs::engine::WaveformRegionBoundary::playbackStart;
    switch (gesture)
    {
        case Gesture::playbackStart: boundary = drs::engine::WaveformRegionBoundary::playbackStart; break;
        case Gesture::playbackEnd: boundary = drs::engine::WaveformRegionBoundary::playbackEnd; break;
        case Gesture::loopStart: boundary = drs::engine::WaveformRegionBoundary::loopStart; break;
        case Gesture::loopEnd: boundary = drs::engine::WaveformRegionBoundary::loopEnd; break;
        default: return;
    }
    const auto regions = drs::engine::normalizeBoundaryDrag(
        currentRegions(), boundary, candidateFrame, preview.frameCount);
    preview.playbackStartFrame = regions.playback.startFrame;
    preview.playbackEndFrameExclusive = regions.playback.endFrameExclusive;
    preview.loopStartFrame = regions.loop.startFrame;
    preview.loopEndFrame = regions.loop.endFrameExclusive;
    const auto resolvedBoundaryFrame = gesture == Gesture::playbackStart
        ? regions.playback.startFrame
        : (gesture == Gesture::playbackEnd
               ? regions.playback.endFrameExclusive
               : (gesture == Gesture::loopStart
                      ? regions.loop.startFrame : regions.loop.endFrameExclusive));
    snapApplied = snapped && resolvedBoundaryFrame != latestRawCandidateFrame;
    if (snapApplied)
        snapStatus = "Snapped " + std::to_string(latestRawCandidateFrame) + " → "
            + std::to_string(resolvedBoundaryFrame) + " · Alt bypasses";
    repaint();
}

void WaveformDetailView::submitSnapCandidate(const std::uint64_t candidateFrame)
{
    if (!zeroCrossingSnapEnabled || preview.sourceIdentity.empty() || preview.sourcePath.empty())
        return;
    WaveformSnapRequest request;
    request.sourceIdentity = preview.sourceIdentity;
    request.sourcePath = preview.sourcePath;
    request.candidateFrame = candidateFrame;
    request.searchRadiusFrames = std::min<std::uint64_t>(2048,
        std::max<std::uint64_t>(32, viewportFrames.length() / 200));
    const auto submitted = snapService.submit(std::move(request));
    if (submitted.accepted)
    {
        pendingSnapGeneration = submitted.identity.generation;
        snapStatus = "Searching near frame " + std::to_string(candidateFrame)
            + " · Alt bypasses";
    }
}

void WaveformDetailView::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    dragging = true;
    dragStartPosition = event.position;
    dragStartViewport = viewportFrames;
    originalLoopStartFrame = preview.loopStartFrame;
    originalLoopEndFrame = preview.loopEndFrame;
    originalPlaybackStartFrame = preview.playbackStartFrame;
    originalPlaybackEndFrame = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    latestRawCandidateFrame = frameAtX(event.position.x);
    snapApplied = false;

    if (!viewportFrames.empty())
    {
        const auto inner = getCanvasBounds();
        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        const auto regions = currentRegions();
        const auto startX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(regions.playback.startFrame, viewport));
        const auto endX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(regions.playback.endFrameExclusive, viewport));
        constexpr auto handleTolerance = 12.0f;
        if (event.position.y <= inner.getY() + 28.0f
            && std::abs(event.position.x - startX) <= handleTolerance)
            gesture = selectedBoundary = Gesture::playbackStart;
        else if (event.position.y <= inner.getY() + 28.0f
                 && std::abs(event.position.x - endX) <= handleTolerance)
            gesture = selectedBoundary = Gesture::playbackEnd;
    }

    if (gesture == Gesture::none && preview.loopEnabled && !viewportFrames.empty())
    {
        const auto inner = getCanvasBounds();
        const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
        const auto startX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(preview.loopStartFrame, viewport));
        const auto endX = inner.getX() + static_cast<float>(
            drs::engine::waveformFrameToPixel(preview.loopEndFrame, viewport));
        constexpr auto handleTolerance = 9.0f;
        if (std::abs(event.position.x - startX) <= handleTolerance)
            gesture = selectedBoundary = Gesture::loopStart;
        else if (std::abs(event.position.x - endX) <= handleTolerance)
            gesture = selectedBoundary = Gesture::loopEnd;
    }

    if (gesture == Gesture::none && event.mods.isShiftDown())
    {
        selectedBoundary = Gesture::none;
        gesture = Gesture::selection;
        gestureAnchorFrame = frameAtX(event.position.x);
        preview.selectionActive = true;
        preview.selectionStartFrame = gestureAnchorFrame;
        preview.selectionEndFrameExclusive = gestureAnchorFrame;
    }
    else if (gesture == Gesture::none && event.mods.isCommandDown()
             && currentRegions().playback.contains(latestRawCandidateFrame))
    {
        selectedBoundary = Gesture::none;
        gesture = Gesture::playbackMove;
        gestureAnchorFrame = latestRawCandidateFrame;
    }
    else if (gesture == Gesture::none)
    {
        selectedBoundary = Gesture::none;
        gesture = Gesture::pan;
    }
}

void WaveformDetailView::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging || dragStartViewport.empty())
        return;
    if (gesture == Gesture::playbackStart || gesture == Gesture::playbackEnd
        || gesture == Gesture::loopStart || gesture == Gesture::loopEnd)
    {
        latestRawCandidateFrame = frameAtX(event.position.x);
        pendingSnapGeneration = 0;
        snapApplied = false;
        applyBoundaryCandidate(latestRawCandidateFrame, false);
        if (zeroCrossingSnapEnabled && !event.mods.isAltDown())
            submitSnapCandidate(latestRawCandidateFrame);
        else if (event.mods.isAltDown())
            snapStatus = "Direct frame " + std::to_string(latestRawCandidateFrame)
                + " · snap bypassed";
        return;
    }
    if (gesture == Gesture::playbackMove)
    {
        const auto frame = frameAtX(event.position.x);
        std::int64_t delta = 0;
        if (frame >= gestureAnchorFrame)
            delta = static_cast<std::int64_t>(std::min<std::uint64_t>(
                frame - gestureAnchorFrame,
                static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())));
        else
            delta = -static_cast<std::int64_t>(std::min<std::uint64_t>(
                gestureAnchorFrame - frame,
                static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())));
        auto regions = drs::engine::movePlaybackRegion(
            { { originalPlaybackStartFrame, originalPlaybackEndFrame },
              { originalLoopStartFrame, originalLoopEndFrame }, preview.loopEnabled },
            delta, preview.frameCount);
        preview.playbackStartFrame = regions.playback.startFrame;
        preview.playbackEndFrameExclusive = regions.playback.endFrameExclusive;
        preview.loopStartFrame = regions.loop.startFrame;
        preview.loopEndFrame = regions.loop.endFrameExclusive;
        repaint();
        return;
    }
    if (gesture == Gesture::selection)
    {
        const auto frame = frameAtX(event.position.x);
        preview.selectionStartFrame = std::min(gestureAnchorFrame, frame);
        preview.selectionEndFrameExclusive = std::max(gestureAnchorFrame, frame);
        repaint();
        return;
    }
    if (gesture != Gesture::pan)
        return;
    const auto width = std::max(1.0f, getCanvasBounds().getWidth());
    const auto deltaPixels = event.position.x - dragStartPosition.x;
    const auto deltaFrames = static_cast<std::int64_t>(std::llround(
        -static_cast<long double>(deltaPixels) * static_cast<long double>(dragStartViewport.length())
        / static_cast<long double>(width)));
    viewportFrames = drs::engine::panWaveformViewport(dragStartViewport,
                                                       deltaFrames,
                                                       preview.frameCount);
    repaint();
}

void WaveformDetailView::mouseUp(const juce::MouseEvent&)
{
    if (!dragging)
        return;
    dragging = false;
    const auto completedGesture = gesture;
    gesture = Gesture::none;
    snapService.cancel("Waveform boundary gesture completed");
    pendingSnapGeneration = 0;
    if ((completedGesture == Gesture::loopStart || completedGesture == Gesture::loopEnd)
        && (preview.loopStartFrame != originalLoopStartFrame
            || preview.loopEndFrame != originalLoopEndFrame))
    {
        if (loopRegionCommitCallback)
            loopRegionCommitCallback(preview.loopStartFrame,
                                     preview.loopEndFrame,
                                     (completedGesture == Gesture::loopStart
                                         ? "Move SFZ loop start" : "Move SFZ loop end")
                                         + std::string(snapApplied ? " (zero-crossing snap)" : ""));
    }
    else if ((completedGesture == Gesture::playbackStart
              || completedGesture == Gesture::playbackEnd
              || completedGesture == Gesture::playbackMove)
             && (preview.playbackStartFrame != originalPlaybackStartFrame
                 || preview.playbackEndFrameExclusive != originalPlaybackEndFrame))
    {
        if (playbackRegionCommitCallback)
        {
            auto label = completedGesture == Gesture::playbackStart
                ? std::string("Move SFZ playback offset")
                : (completedGesture == Gesture::playbackEnd
                       ? std::string("Move SFZ playback end")
                       : std::string("Move SFZ playback region"));
            if (snapApplied)
                label += " (zero-crossing snap)";
            playbackRegionCommitCallback(preview.playbackStartFrame,
                                         preview.playbackEndFrameExclusive,
                                         label);
        }
    }
    else if (completedGesture == Gesture::pan)
    {
        publishDetailRequest();
    }
}

void WaveformDetailView::mouseDoubleClick(const juce::MouseEvent&)
{
    fitToSource(false);
}

void WaveformDetailView::mouseWheelMove(const juce::MouseEvent& event,
                                        const juce::MouseWheelDetails& wheel)
{
    if (preview.frameCount == 0 || viewportFrames.empty())
        return;
    const auto inner = getCanvasBounds();
    const drs::engine::WaveformViewport viewport { viewportFrames, inner.getWidth() };
    const auto anchor = drs::engine::waveformPixelToFrame(event.position.x - inner.getX(), viewport);
    const auto scale = std::pow(1.8, -static_cast<double>(wheel.deltaY));
    viewportFrames = drs::engine::zoomWaveformViewport(viewportFrames,
                                                       anchor,
                                                       scale,
                                                       preview.frameCount,
                                                       32);
    publishDetailRequest();
    repaint();
}

bool WaveformDetailView::keyPressed(const juce::KeyPress& key)
{
    if (preview.frameCount == 0 || viewportFrames.empty())
        return false;
    if (key.getKeyCode() == juce::KeyPress::escapeKey && gesture != Gesture::none)
    {
        cancelRegionGesture();
        return true;
    }

    if ((selectedBoundary == Gesture::playbackStart || selectedBoundary == Gesture::playbackEnd
         || selectedBoundary == Gesture::loopStart || selectedBoundary == Gesture::loopEnd)
        && (key.getKeyCode() == juce::KeyPress::leftKey
            || key.getKeyCode() == juce::KeyPress::rightKey)
        && (selectedBoundary == Gesture::playbackStart
            || selectedBoundary == Gesture::playbackEnd || preview.loopEnabled))
    {
        const auto current = selectedBoundary == Gesture::playbackStart
            ? preview.playbackStartFrame
            : (selectedBoundary == Gesture::playbackEnd
                   ? currentRegions().playback.endFrameExclusive
                   : (selectedBoundary == Gesture::loopStart
                          ? preview.loopStartFrame : preview.loopEndFrame));
        const auto candidate = key.getKeyCode() == juce::KeyPress::leftKey
            ? (current == 0 ? 0 : current - 1)
            : (current == std::numeric_limits<std::uint64_t>::max() ? current : current + 1);
        const auto boundary = selectedBoundary == Gesture::playbackStart
            ? drs::engine::WaveformRegionBoundary::playbackStart
            : (selectedBoundary == Gesture::playbackEnd
                   ? drs::engine::WaveformRegionBoundary::playbackEnd
                   : (selectedBoundary == Gesture::loopStart
                          ? drs::engine::WaveformRegionBoundary::loopStart
                          : drs::engine::WaveformRegionBoundary::loopEnd));
        auto regions = drs::engine::normalizeBoundaryDrag(
            currentRegions(), boundary,
            candidate,
            preview.frameCount);
        preview.playbackStartFrame = regions.playback.startFrame;
        preview.playbackEndFrameExclusive = regions.playback.endFrameExclusive;
        preview.loopStartFrame = regions.loop.startFrame;
        preview.loopEndFrame = regions.loop.endFrameExclusive;
        if ((selectedBoundary == Gesture::loopStart || selectedBoundary == Gesture::loopEnd)
            && loopRegionCommitCallback)
            loopRegionCommitCallback(preview.loopStartFrame, preview.loopEndFrame,
                                     selectedBoundary == Gesture::loopStart
                                         ? "Nudge SFZ loop start" : "Nudge SFZ loop end");
        else if (playbackRegionCommitCallback)
            playbackRegionCommitCallback(preview.playbackStartFrame,
                                         preview.playbackEndFrameExclusive,
                                         selectedBoundary == Gesture::playbackStart
                                             ? "Nudge SFZ playback offset"
                                             : "Nudge SFZ playback end");
        repaint();
        return true;
    }

    const auto step = std::max<std::uint64_t>(1, viewportFrames.length() / 10);
    if (key.getKeyCode() == juce::KeyPress::leftKey)
        panByFrames(-static_cast<std::int64_t>(step), true);
    else if (key.getKeyCode() == juce::KeyPress::rightKey)
        panByFrames(static_cast<std::int64_t>(step), true);
    else if (key.getKeyCode() == juce::KeyPress::homeKey || key.getTextCharacter() == '0')
        fitToSource(false);
    else if (key.getTextCharacter() == '+' || key.getTextCharacter() == '=')
    {
        const auto centre = viewportFrames.startFrame + viewportFrames.length() / 2;
        viewportFrames = drs::engine::zoomWaveformViewport(
            viewportFrames, centre, 0.5, preview.frameCount, 32);
        publishDetailRequest();
        repaint();
    }
    else if (key.getTextCharacter() == '-')
    {
        const auto centre = viewportFrames.startFrame + viewportFrames.length() / 2;
        viewportFrames = drs::engine::zoomWaveformViewport(
            viewportFrames, centre, 2.0, preview.frameCount, 32);
        publishDetailRequest();
        repaint();
    }
    else
        return false;
    return true;
}

void WaveformDetailView::focusLost(FocusChangeType)
{
    cancelRegionGesture();
}

void WaveformDetailView::timerCallback()
{
    if (pendingSnapGeneration == 0 || !dragging
        || !(gesture == Gesture::playbackStart || gesture == Gesture::playbackEnd
             || gesture == Gesture::loopStart || gesture == Gesture::loopEnd))
        return;
    const auto snapshot = snapService.getSnapshot();
    if (snapshot == nullptr || snapshot->identity.generation != pendingSnapGeneration
        || snapshot->stage != WaveformSnapServiceStage::completed)
        return;
    pendingSnapGeneration = 0;
    if (snapshot->decision.applied)
        applyBoundaryCandidate(snapshot->decision.resolvedFrame, true);
    else
    {
        snapStatus = "No zero crossing near frame "
            + std::to_string(latestRawCandidateFrame) + " · direct frame retained";
        repaint();
    }
}
} // namespace drs::app::authoring
