#include "shared/authoring/WaveformDetailView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

namespace drs::app::authoring
{
namespace
{
const auto waveformGrid = visual::mapSurface;
const auto waveformMuted = visual::textMuted;
const auto waveformSelected = visual::modulation;
const auto waveformAccent = visual::selection;
} // namespace

void WaveformDetailView::setPreview(AuthoringWaveformPreview nextPreview)
{
    preview = std::move(nextPreview);
    repaint();
}

void WaveformDetailView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(waveformGrid);
    g.fillRoundedRectangle(bounds, visual::panelRadius);
    g.setColour(visual::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), visual::panelRadius, visual::borderWidth);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(visual::text.withAlpha(0.16f));
    g.drawHorizontalLine(static_cast<int>(inner.getCentreY()), inner.getX(), inner.getRight());

    const auto drawLoopMarkers = [&]
    {
        if (!preview.loopEnabled || preview.frameCount == 0)
            return;

        const auto startX = inner.getX() + inner.getWidth()
            * (static_cast<float>(preview.loopStartFrame) / static_cast<float>(preview.frameCount));
        const auto endX = inner.getX() + inner.getWidth()
            * (static_cast<float>(preview.loopEndFrame) / static_cast<float>(preview.frameCount));
        g.setColour(waveformAccent);
        g.drawVerticalLine(static_cast<int>(startX), inner.getY(), inner.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), inner.getY(), inner.getBottom());
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

    juce::Path waveformPath;
    const auto widthPerPoint = inner.getWidth() / static_cast<float>(preview.points.size());
    for (std::size_t index = 0; index < preview.points.size(); ++index)
    {
        const auto x = inner.getX() + (static_cast<float>(index) + 0.5f) * widthPerPoint;
        const auto minY = juce::jmap(preview.points[index].minValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        const auto maxY = juce::jmap(preview.points[index].maxValue, -1.0f, 1.0f, inner.getBottom(), inner.getY());
        waveformPath.startNewSubPath(x, minY);
        waveformPath.lineTo(x, maxY);
    }

    g.setColour(waveformSelected);
    g.strokePath(waveformPath, juce::PathStrokeType(1.3f));
    drawLoopMarkers();

    if (!preview.state.empty() && preview.state != "Ready")
    {
        g.setColour(waveformMuted);
        g.drawFittedText(juce::String::fromUTF8(preview.state.c_str()),
                         getLocalBounds().reduced(12).removeFromTop(20),
                         juce::Justification::centredLeft,
                         1);
    }
}
} // namespace drs::app::authoring
