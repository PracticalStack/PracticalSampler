#include "../support/OpenWorkbenchUiFixtures.h"
#include "shared/authoring/ZoneMapCanvas.h"

#include <cmath>
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

void requireNear(const float actual,
                 const float expected,
                 const float tolerance,
                 const std::string& message)
{
    require(std::abs(actual - expected) <= tolerance,
            message + " (expected " + std::to_string(expected)
                + ", actual " + std::to_string(actual) + ")");
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto* child : root.getChildren())
        if (auto* match = findDescendantById(*child, id); match != nullptr)
            return match;
    return nullptr;
}

juce::Button& requireButton(juce::Component& root, const juce::String& id)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, id));
    require(button != nullptr, "Missing Zone Map toolbar button: " + id.toStdString());
    return *button;
}

juce::Label& requireLabel(juce::Component& root, const juce::String& id)
{
    auto* label = dynamic_cast<juce::Label*>(findDescendantById(root, id));
    require(label != nullptr, "Missing Zone Map toolbar label: " + id.toStdString());
    return *label;
}

void qualifyToolbarAndFitCommands()
{
    using drs::app::authoring::ZoneMapCanvas;
    auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture();
    ZoneMapCanvas canvas;
    canvas.setSize(1120, 520);
    canvas.setVisible(true);
    canvas.setZoneSummaries(fixture.zones);

    auto& fitAll = requireButton(canvas, "authoringZoneMapFitAll");
    auto& fitSelected = requireButton(canvas, "authoringZoneMapFitSelected");
    auto& zoomOut = requireButton(canvas, "authoringZoneMapZoomOut");
    auto& zoomIn = requireButton(canvas, "authoringZoneMapZoomIn");
    auto& zoomValue = requireLabel(canvas, "authoringZoneMapZoomValue");
    require(findDescendantById(canvas, "authoringZoneMapToolbar") != nullptr,
            "Zone Map navigation commands must live in a named toolbar.");
    require(zoomValue.getText() == "25%"
                && canvas.getDisplayedZoomPercentage() == 25,
            "Fit All must use the approved 25% overview vocabulary.");
    require(!fitAll.isEnabled() && !fitSelected.isEnabled() && !zoomOut.isEnabled()
                && zoomIn.isEnabled(),
            "Toolbar actions must expose correct Fit All enablement.");

    const auto& selectedZone = fixture.zones.front();
    canvas.setSelectionState({ { selectedZone.id }, selectedZone.id });
    require(fitSelected.isEnabled(), "Fit Selected must enable when the map owns a selection.");
    fitSelected.onClick();
    require(canvas.getZoomFactor() > 1.0f
                && canvas.getSelectionState().primaryZoneId == selectedZone.id,
            "Fit Selected must frame the selection without changing it.");
    require(zoomValue.getText() != "25%" && fitAll.isEnabled() && zoomOut.isEnabled(),
            "Toolbar state must refresh after Fit Selected.");

    fitAll.onClick();
    requireNear(canvas.getZoomFactor(), 1.0f, 0.0001f,
                "Fit All must restore the complete normalized map.");
    require(canvas.getViewportOrigin() == juce::Point<float> {}
                && zoomValue.getText() == "25%",
            "Fit All must restore the origin and displayed overview scale.");

    zoomIn.onClick();
    require(canvas.getZoomFactor() > 1.0f && zoomValue.getText() != "25%",
            "The toolbar Zoom In action must update the viewport and readout.");
    zoomOut.onClick();
    requireNear(canvas.getZoomFactor(), 1.0f, 0.0001f,
                "Symmetric toolbar zoom actions must return to Fit All.");

    for (int step = 0; step < 32; ++step)
        zoomIn.onClick();
    require(canvas.getDisplayedZoomPercentage() == 200 && !zoomIn.isEnabled(),
            "Toolbar zoom must clamp at the approved 200% detail scale.");
}

void qualifyViewPreservationAndTopologyReset()
{
    using drs::app::authoring::ZoneMapCanvas;
    auto baseline = drs::test::makeOpenWorkbenchZoneMapFixture();
    ZoneMapCanvas canvas;
    canvas.setSize(960, 440);
    canvas.setZoneSummaries(baseline.zones);
    const auto centre = canvas.getMapViewportBounds().getCentre();
    require(canvas.requestZoomAt(centre, 0.5f),
            "Topology preservation qualification requires a zoomed view.");
    const auto zoomBeforeRefresh = canvas.getZoomFactor();
    const auto originBeforeRefresh = canvas.getViewportOrigin();

    canvas.setZoneSummaries(drs::test::makeOpenWorkbenchZoneMapFixture().zones);
    requireNear(canvas.getZoomFactor(), zoomBeforeRefresh, 0.0001f,
                "Refreshing unchanged zone topology must preserve zoom.");
    require(canvas.getViewportOrigin() == originBeforeRefresh,
            "Refreshing unchanged zone topology must preserve origin.");

    canvas.setZoneSummaries(drs::test::makeOpenWorkbenchZoneMapFixture(1000).zones);
    requireNear(canvas.getZoomFactor(), 1.0f, 0.0001f,
                "A material topology change must return to Fit All.");
    require(canvas.getViewportOrigin() == juce::Point<float> {},
            "A material topology change must reset the viewport origin.");
}

void qualifyWheelAndAxisContract()
{
    using drs::app::authoring::ZoneMapCanvas;
    ZoneMapCanvas canvas;
    canvas.setSize(900, 400);
    canvas.setVisible(true);
    canvas.setZoneSummaries(drs::test::makeOpenWorkbenchZoneMapFixture().zones);
    const auto plot = canvas.getMapViewportBounds();
    require(plot.getX() >= 40.0f && plot.getY() >= 38.0f
                && plot.getBottom() <= canvas.getHeight() - 28.0f,
            "The map plot must reserve pinned velocity and pitch axes plus navigation toolbar space.");

    const auto centre = plot.getCentre();
    require(canvas.requestZoomAt(centre, 0.5f),
            "Smooth-pan qualification requires a zoomed map.");
    const auto originBeforePan = canvas.getViewportOrigin();
    const auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    const auto eventTime = juce::Time::getCurrentTime();
    const juce::MouseEvent smoothEvent(mouseSource,
                                       centre,
                                       juce::ModifierKeys {},
                                       1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                       &canvas, &canvas,
                                       eventTime, centre, eventTime, 1, false);
    juce::MouseWheelDetails smoothWheel {};
    smoothWheel.deltaX = -0.2f;
    smoothWheel.deltaY = -0.1f;
    smoothWheel.isSmooth = true;
    canvas.mouseWheelMove(smoothEvent, smoothWheel);
    require(canvas.getViewportOrigin().x > originBeforePan.x
                && canvas.getViewportOrigin().y > originBeforePan.y,
            "A smooth two-axis wheel gesture must pan naturally instead of zooming.");

    const auto originBeforeHorizontalPan = canvas.getViewportOrigin();
    const juce::MouseEvent shiftEvent(mouseSource,
                                      centre,
                                      juce::ModifierKeys::shiftModifier,
                                      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                      &canvas, &canvas,
                                      eventTime, centre, eventTime, 1, false);
    juce::MouseWheelDetails shiftWheel {};
    shiftWheel.deltaY = 0.1f;
    canvas.mouseWheelMove(shiftEvent, shiftWheel);
    require(canvas.getViewportOrigin().x != originBeforeHorizontalPan.x
                && canvas.getViewportOrigin().y == originBeforeHorizontalPan.y,
            "Shift-wheel must pan horizontally without moving the velocity viewport.");

    juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);
    {
        juce::Graphics graphics(image);
        canvas.paint(graphics);
    }
    const auto screenshot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("drs-open-workbench-phase1-zone-map.png");
    screenshot.deleteFile();
    if (auto stream = screenshot.createOutputStream(); stream != nullptr)
    {
        juce::PNGImageFormat png;
        require(png.writeImageToStream(image, *stream),
                "The Phase 1 map visual-check artifact must be writable.");
    }
    else
    {
        require(false, "The Phase 1 map visual-check artifact could not be created.");
    }
}

void qualifyFitAllOuterEdges()
{
    using drs::app::authoring::ZoneMapCanvas;
    drs::engine::AuthoringZoneSummary lowEdge;
    lowEdge.id = "fit-all-low-edge";
    lowEdge.displayName = "Low edge";
    lowEdge.rootKey = 0;
    lowEdge.keyLow = 0;
    lowEdge.keyHigh = 0;
    lowEdge.velocityLow = 1;
    lowEdge.velocityHigh = 127;
    auto highEdge = lowEdge;
    highEdge.id = "fit-all-high-edge";
    highEdge.displayName = "High edge";
    highEdge.rootKey = 127;
    highEdge.keyLow = 127;
    highEdge.keyHigh = 127;

    ZoneMapCanvas canvas;
    canvas.setSize(900, 400);
    canvas.setZoneSummaries({ lowEdge, highEdge });
    std::string selectedId;
    canvas.setOnZoneSelectionStateRequested(
        [&](const ZoneMapCanvas::SelectionState& selection) { selectedId = selection.primaryZoneId; });
    const auto plot = canvas.getMapViewportBounds();
    require(canvas.requestSelectionAt({ plot.getX() + plot.getWidth() * (127.5f / 128.0f),
                                        plot.getCentreY() }),
            "Fit All must keep the MIDI 127 zone inside the selectable viewport.");
    require(selectedId == highEdge.id,
            "The outermost high-key zone must not be clipped or replaced by its neighbour.");
    require(canvas.requestSelectionAt({ plot.getX() + plot.getWidth() * (0.5f / 128.0f),
                                        plot.getCentreY() }),
            "Fit All must keep the MIDI 0 zone inside the selectable viewport.");
    require(selectedId == lowEdge.id,
            "The outermost low-key zone must remain selectable at Fit All.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyToolbarAndFitCommands();
        qualifyViewPreservationAndTopologyReset();
        qualifyWheelAndAxisContract();
        qualifyFitAllOuterEdges();
        std::cout << "Open Workbench Phase 1 tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 1 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
