#include "../support/OpenWorkbenchUiFixtures.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "shared/authoring/ZoneMapViewState.h"

#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(const float actual,
                 const float expected,
                 const float tolerance,
                 const std::string& message)
{
    require(std::abs(actual - expected) <= tolerance,
            message + " (expected " + std::to_string(expected)
                + ", actual " + std::to_string(actual) + ")");
}

void qualifyFixture(const std::size_t zoneCount)
{
    const auto first = drs::test::makeOpenWorkbenchZoneMapFixture(zoneCount);
    const auto second = drs::test::makeOpenWorkbenchZoneMapFixture(zoneCount);
    require(first.zones.size() == zoneCount
                && first.groupIdsByZone.size() == zoneCount,
            "Open Workbench fixture must return the requested zone and group counts.");
    require(second.zones.size() == first.zones.size(),
            "Repeated fixture generation must retain its size.");

    std::set<std::string> ids;
    std::set<std::string> groups;
    std::size_t oneKeyZones = 0;
    std::size_t longNames = 0;
    std::size_t crossfadedZones = 0;
    std::size_t identicalFullRangeZones = 0;
    for (std::size_t index = 0; index < first.zones.size(); ++index)
    {
        const auto& zone = first.zones[index];
        const auto& repeated = second.zones[index];
        require(zone.id == repeated.id
                    && zone.displayName == repeated.displayName
                    && zone.keyLow == repeated.keyLow
                    && zone.keyHigh == repeated.keyHigh
                    && zone.velocityLow == repeated.velocityLow
                    && zone.velocityHigh == repeated.velocityHigh
                    && first.groupIdsByZone[index] == second.groupIdsByZone[index],
                "Open Workbench fixture generation must be deterministic at every index.");
        require(zone.keyLow >= 0 && zone.keyHigh <= 127 && zone.keyLow <= zone.keyHigh,
                "Fixture key ranges must remain valid.");
        require(zone.velocityLow >= 1 && zone.velocityHigh <= 127
                    && zone.velocityLow <= zone.velocityHigh,
                "Fixture velocity ranges must remain valid.");
        ids.insert(zone.id);
        groups.insert(first.groupIdsByZone[index]);
        oneKeyZones += zone.keyLow == zone.keyHigh ? 1u : 0u;
        longNames += zone.displayName.size() >= 64u ? 1u : 0u;
        crossfadedZones += drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade) ? 1u : 0u;
        identicalFullRangeZones += zone.keyLow == 0 && zone.keyHigh == 127
                && zone.velocityLow == 1 && zone.velocityHigh == 127
            ? 1u : 0u;
    }

    require(ids.size() == zoneCount, "Every fixture zone must have a unique stable ID.");
    require(groups.size() == 12u, "Large UI fixtures must expose twelve deterministic zone groups.");
    require(oneKeyZones >= zoneCount - 2u,
            "The baseline fixture must predominantly exercise narrow one-key zones.");
    require(longNames > 0u, "The baseline fixture must exercise long-name truncation.");
    require(crossfadedZones > zoneCount / 2u,
            "The baseline fixture must exercise dense overlapping velocity layers.");
    require(identicalFullRangeZones == 2u,
            "The baseline fixture must retain its pathological identical overlap pair.");
}

void qualifyViewState()
{
    using drs::app::authoring::ZoneMapViewState;
    ZoneMapViewState view;
    requireNear(view.getZoom(), 1.0f, 0.0001f, "Zone Map view must start at Fit All zoom.");
    require(view.getOrigin() == juce::Point<float> {},
            "Zone Map view must start at the normalized origin.");

    const juce::Point<float> anchor { 0.37f, 0.64f };
    const auto contentBeforeZoom = view.viewportToContent(anchor);
    require(view.zoomAt(anchor, 0.5f), "A positive wheel delta must zoom the view.");
    const auto contentAfterZoom = view.viewportToContent(anchor);
    requireNear(contentAfterZoom.x, contentBeforeZoom.x, 0.00001f,
                "Pointer-centred zoom must preserve the horizontal content anchor.");
    requireNear(contentAfterZoom.y, contentBeforeZoom.y, 0.00001f,
                "Pointer-centred zoom must preserve the vertical content anchor.");

    const juce::Point<float> contentPoint { 0.42f, 0.77f };
    const auto roundTrip = view.viewportToContent(view.contentToViewport(contentPoint));
    requireNear(roundTrip.x, contentPoint.x, 0.00001f,
                "Content/view transforms must round-trip horizontally.");
    requireNear(roundTrip.y, contentPoint.y, 0.00001f,
                "Content/view transforms must round-trip vertically.");

    require(view.panByPixels({ -100000.0f, -100000.0f }, { 800.0f, 320.0f }),
            "A zoomed view must support programmatic pan.");
    const auto maximumOrigin = 1.0f - 1.0f / view.getZoom();
    requireNear(view.getOrigin().x, maximumOrigin, 0.00001f,
                "Horizontal pan must clamp to the content edge.");
    requireNear(view.getOrigin().y, maximumOrigin, 0.00001f,
                "Vertical pan must clamp to the content edge.");

    require(view.fitContentBounds({ 0.25f, 0.2f, 0.5f, 0.4f }, 0.05f),
            "A non-empty content rectangle must produce a fitted view.");
    requireNear(view.getZoom(), 1.0f / 0.6f, 0.0001f,
                "Fit bounds must choose the limiting padded extent.");
    requireNear(view.getOrigin().x, 0.2f, 0.0001f,
                "Fit bounds must center the horizontal selection.");
    requireNear(view.getOrigin().y, 0.1f, 0.0001f,
                "Fit bounds must center the vertical selection.");

    require(view.fitContentBounds({ 0.49f, 0.49f, 0.01f, 0.01f }),
            "A small content rectangle must remain fit-capable.");
    requireNear(view.getZoom(), ZoneMapViewState::maximumZoom, 0.0001f,
                "Fit bounds must clamp to the maximum supported zoom.");
    view.reset();
    require(view.getOrigin() == juce::Point<float> {} && view.getZoom() == 1.0f,
            "Reset must restore the complete normalized extent.");
}

void qualifyCanvasIntegration()
{
    auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture();
    drs::app::authoring::ZoneMapCanvas canvas;
    canvas.setSize(1120, 520);
    canvas.setVisible(true);
    canvas.setZoneSummaries(fixture.zones);
    canvas.setSelectionState({ { fixture.zones.front().id }, fixture.zones.front().id });

    juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);
    juce::Graphics graphics(image);
    canvas.paintEntireComponent(graphics, true);

    const auto centre = canvas.getLocalBounds().getCentre().toFloat();
    require(canvas.requestZoomAt(centre, 0.5f),
            "The canvas must delegate existing pointer zoom to the view-state model.");
    require(canvas.requestPanBy({ -80.0f, -30.0f }),
            "The canvas must delegate existing pixel pan to the view-state model.");
    canvas.resetViewport();
    requireNear(canvas.getZoomFactor(), 1.0f, 0.0001f,
                "Canvas reset must preserve the pre-iteration viewport contract.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyFixture(642);
        qualifyFixture(1000);
        qualifyViewState();
        qualifyCanvasIntegration();
        std::cout << "Open Workbench Phase 0 tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 0 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
