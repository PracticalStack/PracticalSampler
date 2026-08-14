#include "../support/OpenWorkbenchUiFixtures.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "shared/authoring/ZoneMapRenderPolicy.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::MouseEvent makeMouseEvent(juce::Component& component,
                                const juce::Point<float> position,
                                const juce::ModifierKeys modifiers)
{
    const auto source = juce::Desktop::getInstance().getMainMouseSource();
    const auto now = juce::Time::getCurrentTime();
    return { source, position, modifiers,
             1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             &component, &component, now, position, now, 1, false };
}

void qualifySemanticZoomPolicy()
{
    using drs::app::authoring::ZoneMapDetailLevel;
    using drs::app::authoring::ZoneMapRenderPolicy;
    const auto overview = ZoneMapRenderPolicy::forDisplayedZoom(34);
    const auto working = ZoneMapRenderPolicy::forDisplayedZoom(35);
    const auto detail = ZoneMapRenderPolicy::forDisplayedZoom(90);

    require(overview.level == ZoneMapDetailLevel::overview
                && overview.drawSelectionAggregate
                && !overview.drawSelectedLabel
                && !overview.drawRangeHandles
                && !overview.drawCrossfades,
            "Overview must suppress labels, handles, and crossfade decoration.");
    require(working.level == ZoneMapDetailLevel::working
                && working.drawSelectedLabel
                && working.drawHoverLabel
                && working.drawRangeHandles
                && !working.drawCrossfadeHandles,
            "Working scale must expose selection/hover labels and range handles only.");
    require(detail.level == ZoneMapDetailLevel::detail
                && detail.drawCrossfadeHandles
                && !detail.drawSelectionAggregate,
            "Detail scale must expose crossfade editing without the overview aggregate.");
}

void qualifyCacheCullingAndMinimap()
{
    using drs::app::authoring::ZoneMapCanvas;
    using drs::app::authoring::ZoneMapDetailLevel;
    auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture(1000);
    ZoneMapCanvas canvas;
    canvas.setSize(1120, 520);
    canvas.setVisible(true);
    canvas.setZoneSummaries(fixture.zones);

    require(canvas.getCachedGeometryCount() == 1000u,
            "Every zone must have one cached normalized geometry record.");
    require(canvas.getDetailLevel() == ZoneMapDetailLevel::overview,
            "Fit All must begin at semantic Overview scale.");
    require(canvas.getOverview().getComponentID() == "authoringZoneMapMinimap"
                && canvas.getOverview().isVisible()
                && canvas.getMinimapBounds().getWidth() >= 132,
            "The persistent minimap must be a named, visible, usable child control.");
    require(canvas.getOverview().getGroupCount() == 12u,
            "The minimap must aggregate the fixture into its twelve stable groups.");

    const std::vector<std::string> selectedIds {
        fixture.zones[20].id, fixture.zones[42].id, fixture.zones[63].id
    };
    canvas.setSelectionState({ selectedIds, selectedIds.front() });
    require(canvas.getOverview().getSelectedZoneCount() == selectedIds.size(),
            "Minimap selection must remain synchronized with the main map.");

    const auto plotCentre = canvas.getMapViewportBounds().getCentre();
    require(canvas.requestZoomAt(plotCentre, 0.28f)
                && canvas.getDetailLevel() == ZoneMapDetailLevel::working,
            "Crossing 35% must switch the map into Working scale.");
    require(canvas.requestZoomAt(plotCentre, 1.0f)
                && canvas.getDetailLevel() == ZoneMapDetailLevel::detail,
            "Crossing 90% must switch the map into Detail scale.");

    juce::Image paintTarget(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);
    {
        juce::Graphics graphics(paintTarget);
        canvas.paint(graphics);
    }
    require(canvas.getLastVisibleZoneCount() > 0u
                && canvas.getLastVisibleZoneCount() < canvas.getCachedGeometryCount(),
            "Detail paint must cull normalized geometry outside the visible viewport.");

    const auto originBeforeMinimap = canvas.getViewportOrigin();
    auto& minimap = canvas.getOverview();
    const auto clickPosition = minimap.getLocalBounds().toFloat().reduced(8.0f).getBottomRight()
        - juce::Point<float> { 8.0f, 8.0f };
    const auto down = makeMouseEvent(minimap, clickPosition, juce::ModifierKeys::leftButtonModifier);
    minimap.mouseDown(down);
    minimap.mouseUp(down);
    require(canvas.getViewportOrigin() != originBeforeMinimap,
            "Clicking the minimap must recenter the main viewport.");
}

void qualifyDensePaintBudgetAndArtifact()
{
    using drs::app::authoring::ZoneMapCanvas;
    ZoneMapCanvas canvas;
    canvas.setSize(1120, 520);
    canvas.setVisible(true);
    auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture(1000);
    canvas.setZoneSummaries(fixture.zones);
    canvas.setSelectionState({ { fixture.zones.front().id }, fixture.zones.front().id });
    juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);

    constexpr int iterationCount = 30;
    const auto started = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < iterationCount; ++iteration)
    {
        image.clear(image.getBounds(), juce::Colours::transparentBlack);
        juce::Graphics graphics(image);
        canvas.paint(graphics);
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    const auto averagePaintMilliseconds = elapsed / iterationCount;
    std::cout << "Phase 2 dense overview average paint: "
              << averagePaintMilliseconds << " ms (qualification build)\n";
    require(averagePaintMilliseconds < 25.0,
            "A 1,000-zone overview paint must stay inside the 25 ms Debug qualification ceiling.");

    const auto snapshot = canvas.createComponentSnapshot(canvas.getLocalBounds(), true, 1.0f);
    const auto screenshot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("drs-open-workbench-phase2-dense-map.png");
    screenshot.deleteFile();
    auto stream = screenshot.createOutputStream();
    require(stream != nullptr, "The Phase 2 visual-check artifact must be writable.");
    juce::PNGImageFormat png;
    require(png.writeImageToStream(snapshot, *stream),
            "The Phase 2 visual-check artifact must encode as PNG.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifySemanticZoomPolicy();
        qualifyCacheCullingAndMinimap();
        qualifyDensePaintBudgetAndArtifact();
        std::cout << "Open Workbench Phase 2 tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 2 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
