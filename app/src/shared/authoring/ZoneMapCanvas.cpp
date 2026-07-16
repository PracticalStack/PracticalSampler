#include "shared/authoring/ZoneMapCanvas.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
const auto zoneMapGrid = juce::Colour::fromRGB(230, 220, 207);
const auto zoneMapSelected = juce::Colour::fromRGB(28, 108, 88);
const auto zoneMapAccent = juce::Colour::fromRGB(181, 96, 21);
} // namespace

void ZoneMapCanvas::setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries)
{
    zoneSummaries = std::move(summaries);
    repaint();
}

void ZoneMapCanvas::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::transparentBlack);
    g.setColour(zoneMapGrid);
    g.fillRoundedRectangle(bounds, 14.0f);

    const auto inner = bounds.reduced(12.0f);
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 24));

    for (int key = 0; key <= 8; ++key)
    {
        const auto x = inner.getX() + (inner.getWidth() * static_cast<float>(key) / 8.0f);
        g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
    }

    for (int velocity = 0; velocity <= 4; ++velocity)
    {
        const auto y = inner.getY() + (inner.getHeight() * static_cast<float>(velocity) / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), inner.getX(), inner.getRight());
    }

    for (const auto& zone : zoneSummaries)
    {
        const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
        const auto width = std::max(10.0f,
                                    inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
        const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
        const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
        const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
        const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));

        const juce::Rectangle<float> zoneBounds(x, y, width, height);
        g.setColour(zone.selected ? zoneMapSelected : zoneMapAccent.withMultipliedAlpha(0.72f));
        g.fillRoundedRectangle(zoneBounds, 8.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(zone.displayName.c_str()),
                         zoneBounds.toNearestInt().reduced(6, 4),
                         juce::Justification::centredLeft,
                         1);
    }
}
} // namespace drs::app::authoring
