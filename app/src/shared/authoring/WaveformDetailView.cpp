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
    if (gesture == Gesture::loopStart || gesture == Gesture::loopEnd)
    {
        preview.loopStartFrame = originalLoopStartFrame;
        preview.loopEndFrame = originalLoopEndFrame;
    }
    gesture = Gesture::none;
    dragging = false;
    repaint();
}

void WaveformDetailView::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    dragging = true;
    dragStartPosition = event.position;
    dragStartViewport = viewportFrames;
    originalLoopStartFrame = preview.loopStartFrame;
    originalLoopEndFrame = preview.loopEndFrame;

    if (preview.loopEnabled && !viewportFrames.empty())
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
    if (gesture == Gesture::loopStart || gesture == Gesture::loopEnd)
    {
        drs::engine::WaveformEditableRegions regions {
            { preview.playbackStartFrame, preview.frameCount },
            { preview.loopStartFrame, preview.loopEndFrame },
            preview.loopEnabled
        };
        if (regions.playback.startFrame >= regions.playback.endFrameExclusive)
            regions.playback = { 0, preview.frameCount };
        regions = drs::engine::normalizeBoundaryDrag(
            regions,
            gesture == Gesture::loopStart
                ? drs::engine::WaveformRegionBoundary::loopStart
                : drs::engine::WaveformRegionBoundary::loopEnd,
            frameAtX(event.position.x),
            preview.frameCount);
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
    if ((completedGesture == Gesture::loopStart || completedGesture == Gesture::loopEnd)
        && (preview.loopStartFrame != originalLoopStartFrame
            || preview.loopEndFrame != originalLoopEndFrame))
    {
        if (loopRegionCommitCallback)
            loopRegionCommitCallback(preview.loopStartFrame,
                                     preview.loopEndFrame,
                                     completedGesture == Gesture::loopStart
                                         ? "Move SFZ loop start" : "Move SFZ loop end");
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

    if ((selectedBoundary == Gesture::loopStart || selectedBoundary == Gesture::loopEnd)
        && (key.getKeyCode() == juce::KeyPress::leftKey
            || key.getKeyCode() == juce::KeyPress::rightKey)
        && preview.loopEnabled)
    {
        const auto current = selectedBoundary == Gesture::loopStart
            ? preview.loopStartFrame : preview.loopEndFrame;
        const auto candidate = key.getKeyCode() == juce::KeyPress::leftKey
            ? (current == 0 ? 0 : current - 1)
            : (current == std::numeric_limits<std::uint64_t>::max() ? current : current + 1);
        auto regions = drs::engine::normalizeBoundaryDrag(
            { { preview.playbackStartFrame, preview.frameCount },
              { preview.loopStartFrame, preview.loopEndFrame }, true },
            selectedBoundary == Gesture::loopStart
                ? drs::engine::WaveformRegionBoundary::loopStart
                : drs::engine::WaveformRegionBoundary::loopEnd,
            candidate,
            preview.frameCount);
        preview.loopStartFrame = regions.loop.startFrame;
        preview.loopEndFrame = regions.loop.endFrameExclusive;
        if (loopRegionCommitCallback)
            loopRegionCommitCallback(preview.loopStartFrame, preview.loopEndFrame,
                                     selectedBoundary == Gesture::loopStart
                                         ? "Nudge SFZ loop start" : "Nudge SFZ loop end");
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
} // namespace drs::app::authoring
