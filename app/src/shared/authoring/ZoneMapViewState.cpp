#include "shared/authoring/ZoneMapViewState.h"

#include <algorithm>
#include <cmath>

namespace drs::app::authoring
{
int ZoneMapViewState::getDisplayedZoomPercentage() const noexcept
{
    // The design scale presents Fit All as the 25% overview and the existing
    // 8x transform as 200% detail. This changes only user-facing vocabulary.
    return static_cast<int>(std::lround(zoom * 25.0f));
}

juce::Rectangle<float> ZoneMapViewState::getVisibleContentBounds() const noexcept
{
    const auto extent = 1.0f / zoom;
    return { origin.x, origin.y, extent, extent };
}

juce::Point<float> ZoneMapViewState::contentToViewport(
    const juce::Point<float> contentPosition) const noexcept
{
    return (contentPosition - origin) * zoom;
}

juce::Point<float> ZoneMapViewState::viewportToContent(
    const juce::Point<float> viewportPosition) const noexcept
{
    return origin + viewportPosition / zoom;
}

bool ZoneMapViewState::zoomAt(juce::Point<float> viewportPosition,
                              const float wheelDelta) noexcept
{
    if (juce::approximatelyEqual(wheelDelta, 0.0f))
        return false;

    viewportPosition.x = juce::jlimit(0.0f, 1.0f, viewportPosition.x);
    viewportPosition.y = juce::jlimit(0.0f, 1.0f, viewportPosition.y);
    const auto nextZoom = juce::jlimit(
        minimumZoom,
        maximumZoom,
        zoom * std::exp(wheelDelta * zoomSensitivity));
    if (juce::approximatelyEqual(nextZoom, zoom))
        return false;

    const auto contentAtPointer = viewportToContent(viewportPosition);
    zoom = nextZoom;
    origin = clampOrigin(contentAtPointer - viewportPosition / zoom, zoom);
    if (juce::approximatelyEqual(zoom, minimumZoom))
        reset();
    return true;
}

bool ZoneMapViewState::panByPixels(const juce::Point<float> pixelDelta,
                                   const juce::Point<float> viewportSize) noexcept
{
    if (zoom <= minimumZoom
        || viewportSize.x <= 0.0f
        || viewportSize.y <= 0.0f
        || (juce::approximatelyEqual(pixelDelta.x, 0.0f)
            && juce::approximatelyEqual(pixelDelta.y, 0.0f)))
    {
        return false;
    }

    const auto nextOrigin = clampOrigin(
        origin - juce::Point<float> {
            pixelDelta.x / (viewportSize.x * zoom),
            pixelDelta.y / (viewportSize.y * zoom)
        },
        zoom);
    if (nextOrigin == origin)
        return false;

    origin = nextOrigin;
    return true;
}

bool ZoneMapViewState::fitContentBounds(juce::Rectangle<float> contentBounds,
                                        const float paddingProportion) noexcept
{
    contentBounds = contentBounds.getIntersection({ 0.0f, 0.0f, 1.0f, 1.0f });
    if (contentBounds.isEmpty())
        return false;

    const auto padding = std::max(0.0f, paddingProportion);
    const auto paddedWidth = std::min(1.0f, contentBounds.getWidth() + padding * 2.0f);
    const auto paddedHeight = std::min(1.0f, contentBounds.getHeight() + padding * 2.0f);
    const auto nextZoom = juce::jlimit(
        minimumZoom,
        maximumZoom,
        std::min(1.0f / std::max(paddedWidth, 1.0f / maximumZoom),
                 1.0f / std::max(paddedHeight, 1.0f / maximumZoom)));
    const auto visibleExtent = 1.0f / nextZoom;
    const auto nextOrigin = juce::Point<float> {
        contentBounds.getCentreX() - visibleExtent * 0.5f,
        contentBounds.getCentreY() - visibleExtent * 0.5f
    };
    return setView(nextZoom, nextOrigin);
}

bool ZoneMapViewState::setView(const float nextZoom,
                               const juce::Point<float> nextOrigin) noexcept
{
    const auto clampedZoom = juce::jlimit(minimumZoom, maximumZoom, nextZoom);
    const auto clampedOrigin = clampOrigin(nextOrigin, clampedZoom);
    if (juce::approximatelyEqual(clampedZoom, zoom) && clampedOrigin == origin)
        return false;

    zoom = clampedZoom;
    origin = clampedOrigin;
    if (juce::approximatelyEqual(zoom, minimumZoom))
        reset();
    return true;
}

void ZoneMapViewState::reset() noexcept
{
    zoom = minimumZoom;
    origin = {};
}

juce::Point<float> ZoneMapViewState::clampOrigin(juce::Point<float> candidate,
                                                 const float forZoom) const noexcept
{
    const auto maximumOrigin = std::max(0.0f, 1.0f - 1.0f / forZoom);
    candidate.x = juce::jlimit(0.0f, maximumOrigin, candidate.x);
    candidate.y = juce::jlimit(0.0f, maximumOrigin, candidate.y);
    return candidate;
}
} // namespace drs::app::authoring
