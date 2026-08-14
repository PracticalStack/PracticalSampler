#pragma once

#include <juce_graphics/juce_graphics.h>

namespace drs::app::authoring
{
// Deterministic normalized view math for the Zone Map. Content and viewport
// coordinates both use [0, 1] extents; the JUCE component owns pixel conversion
// and gesture arbitration.
class ZoneMapViewState
{
public:
    static constexpr float minimumZoom = 1.0f;
    static constexpr float maximumZoom = 8.0f;
    static constexpr float zoomSensitivity = 1.5f;

    float getZoom() const noexcept { return zoom; }
    int getDisplayedZoomPercentage() const noexcept;
    juce::Point<float> getOrigin() const noexcept { return origin; }
    juce::Rectangle<float> getVisibleContentBounds() const noexcept;

    juce::Point<float> contentToViewport(juce::Point<float> contentPosition) const noexcept;
    juce::Point<float> viewportToContent(juce::Point<float> viewportPosition) const noexcept;

    bool zoomAt(juce::Point<float> viewportPosition, float wheelDelta) noexcept;
    bool panByPixels(juce::Point<float> pixelDelta, juce::Point<float> viewportSize) noexcept;
    bool fitContentBounds(juce::Rectangle<float> contentBounds,
                          float paddingProportion = 0.0f) noexcept;
    bool setView(float nextZoom, juce::Point<float> nextOrigin) noexcept;
    void reset() noexcept;

private:
    juce::Point<float> clampOrigin(juce::Point<float> candidate, float forZoom) const noexcept;

    float zoom = minimumZoom;
    juce::Point<float> origin;
};
} // namespace drs::app::authoring
