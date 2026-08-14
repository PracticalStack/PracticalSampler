#include "../support/OpenWorkbenchUiFixtures.h"
#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/WorkbenchLayoutState.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "standalone/MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
using Clock = std::chrono::steady_clock;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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

juce::Component& requireComponent(juce::Component& root, const juce::String& id)
{
    auto* component = findDescendantById(root, id);
    require(component != nullptr, "Missing component: " + id.toStdString());
    return *component;
}

juce::Button& requireButton(juce::Component& root, const juce::String& id)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, id));
    require(button != nullptr, "Missing button: " + id.toStdString());
    return *button;
}

void requireInside(juce::Component& root, juce::Component& child, const std::string& context)
{
    const auto bounds = root.getLocalArea(&child, child.getLocalBounds());
    require(root.getLocalBounds().contains(bounds), context + " escaped the supported shell bounds.");
}

void qualifyZoneAndGeometryMatrix()
{
    using drs::app::authoring::ZoneMapCanvas;
    for (const auto zoneCount : { 0u, 1u, 24u, 642u, 1000u })
    {
        ZoneMapCanvas canvas;
        canvas.setSize(1120, 520);
        canvas.setVisible(true);
        auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture(zoneCount);
        canvas.setZoneSummaries(fixture.zones);
        require(canvas.getCachedGeometryCount() == zoneCount,
                "Zone-count matrix did not retain exactly " + std::to_string(zoneCount) + " cached zones.");

        juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);
        {
            juce::Graphics graphics(image);
            canvas.paint(graphics);
        }
        require(canvas.getDisplayedZoomPercentage() == 25,
                "Every zone-count fixture must begin at Fit All.");
    }

    auto fixture = drs::test::makeOpenWorkbenchZoneMapFixture(24);
    auto singleVelocity = fixture.zones.front();
    singleVelocity.id = "phase5-single-velocity";
    singleVelocity.velocityLow = 64;
    singleVelocity.velocityHigh = 64;
    fixture.zones.push_back(singleVelocity);
    fixture.zones.push_back(singleVelocity);
    fixture.zones.back().id = "phase5-identical-bounds";

    ZoneMapCanvas canvas;
    canvas.setSize(920, 420);
    canvas.setZoneSummaries(fixture.zones);
    const std::vector<std::string> discontiguous {
        fixture.zones[2].id, fixture.zones[11].id, fixture.zones.back().id
    };
    canvas.setSelectionState({ discontiguous, discontiguous.front() });
    require(canvas.fitSelected() && canvas.getZoomFactor() > 1.0f,
            "A discontiguous selection must be frameable without losing selection.");
    const auto selectionBeforeNavigation = canvas.getSelectionState();
    require(canvas.requestZoomAt(canvas.getMapViewportBounds().getCentre(), 1.0f),
            "Geometry matrix must reach Detail scale.");
    canvas.setSize(760, 320);
    canvas.resized();
    require(canvas.getSelectionState().zoneIds == selectionBeforeNavigation.zoneIds
                && canvas.getViewportOrigin().x >= 0.0f && canvas.getViewportOrigin().y >= 0.0f,
            "Resize while zoomed must clamp the view without mutating selection.");
}

void qualifyMapResponsiveness()
{
    using drs::app::authoring::ZoneMapCanvas;
    ZoneMapCanvas canvas;
    canvas.setSize(1120, 520);
    canvas.setZoneSummaries(drs::test::makeOpenWorkbenchZoneMapFixture(1000).zones);
    juce::Image image(juce::Image::ARGB, canvas.getWidth(), canvas.getHeight(), true);

    constexpr int paintIterations = 40;
    const auto paintStarted = Clock::now();
    for (int iteration = 0; iteration < paintIterations; ++iteration)
    {
        juce::Graphics graphics(image);
        canvas.paint(graphics);
    }
    const auto paintElapsed = std::chrono::duration<double, std::milli>(Clock::now() - paintStarted).count();
    const auto averagePaintMs = paintElapsed / paintIterations;

    const auto inputStarted = Clock::now();
    for (int iteration = 0; iteration < 80; ++iteration)
    {
        canvas.requestZoomAt(canvas.getMapViewportBounds().getCentre(), iteration % 2 == 0 ? 0.08f : -0.08f);
        canvas.requestPanBy({ iteration % 2 == 0 ? 8.0f : -8.0f, iteration % 3 == 0 ? 4.0f : -4.0f });
    }
    const auto averageInputMs = std::chrono::duration<double, std::milli>(Clock::now() - inputStarted).count() / 160.0;
    std::cout << "Phase 5 map profile: averagePaintMs=" << averagePaintMs
              << " averageInputMs=" << averageInputMs << " zones=1000\n";
    require(averagePaintMs < 25.0,
            "The 1,000-zone Debug paint average exceeded the 25 ms qualification ceiling.");
    require(averageInputMs < 4.0,
            "Map zoom/pan request handling exceeded the 4 ms average qualification ceiling.");
}

void qualifyAccessibilityAndKeyboardNavigation()
{
    using drs::app::authoring::ZoneMapCanvas;
    ZoneMapCanvas canvas;
    canvas.setSize(900, 400);
    canvas.setZoneSummaries(drs::test::makeOpenWorkbenchZoneMapFixture(642).zones);
    for (const auto* id : { "authoringZoneMapFitAll", "authoringZoneMapFitSelected",
                            "authoringZoneMapZoomOut", "authoringZoneMapZoomIn" })
    {
        auto& button = requireButton(canvas, id);
        require(button.getTitle().isNotEmpty() && button.getTooltip().isNotEmpty()
                    && button.getExplicitFocusOrder() > 0,
                std::string { id } + " needs a name, tooltip, and explicit focus order.");
    }

    auto& overview = canvas.getOverview();
    require(overview.getTitle().isNotEmpty() && overview.getDescription().isNotEmpty()
                && overview.getHelpText().isNotEmpty() && overview.getWantsKeyboardFocus()
                && overview.getExplicitFocusOrder() > 0,
            "The minimap needs complete accessible metadata and keyboard focus.");
    canvas.requestZoomAt(canvas.getMapViewportBounds().getCentre(), 0.8f);
    const auto originBefore = canvas.getViewportOrigin();
    require(overview.keyPressed(juce::KeyPress(juce::KeyPress::rightKey, {}, 0))
                && canvas.getViewportOrigin().x > originBefore.x,
            "A keyboard-focused minimap must pan the main viewport with arrow keys.");
}

void qualifyAuthoringShellLayouts()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 5 shell qualification requires the reference project.");
    drs::engine::AuthoringSession session(loaded.project);

    drs::app::AuthoringPanel expanded(session, {}, {}, {}, drs::app::AuthoringPanel::LayoutMode::expanded);
    expanded.setVisible(true);
    for (const auto size : { juce::Point<int> { 900, 700 }, { 1120, 800 }, { 1120, 620 } })
    {
        expanded.setSize(size.x, size.y);
        expanded.resized();
        auto& map = requireComponent(expanded, "authoringZoneMap");
        auto& workbench = requireComponent(expanded, "authoringWorkbench");
        requireInside(expanded, map, "Expanded map");
        requireInside(expanded, workbench, "Expanded workbench");
        require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
                "Expanded shell matrix clipped the protected map height.");
    }

    for (const auto* id : { "authoringWorkbenchWaveformTab", "authoringWorkbenchGroupsTab",
                            "authoringWorkbenchMacrosTab", "authoringWorkbenchRoutingTab",
                            "authoringWorkbenchPerformanceTab", "authoringWorkbenchArticulationsTab" })
    {
        auto& tab = requireButton(expanded, id);
        require(tab.getTitle().isNotEmpty() && tab.getDescription().isNotEmpty()
                    && tab.getHelpText().isNotEmpty() && tab.getExplicitFocusOrder() > 0,
                std::string { id } + " failed the workbench accessibility audit.");
        tab.onClick();
        require(tab.getToggleState(), std::string { id } + " did not become the active workbench tab.");
    }

    auto& splitter = requireComponent(expanded, "authoringWorkbenchSplitter");
    require(splitter.getTitle().isNotEmpty() && splitter.getDescription().isNotEmpty()
                && splitter.getHelpText().isNotEmpty() && splitter.getExplicitFocusOrder() > 0,
            "The workbench splitter failed the accessibility audit.");

    for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
    {
        juce::Image image(juce::Image::ARGB,
                          juce::roundToInt(expanded.getWidth() * scale),
                          juce::roundToInt(expanded.getHeight() * scale), true);
        {
            juce::Graphics graphics(image);
            graphics.addTransform(juce::AffineTransform::scale(scale));
            expanded.paint(graphics);
        }
        require(image.getWidth() == juce::roundToInt(expanded.getWidth() * scale)
                    && image.getHeight() == juce::roundToInt(expanded.getHeight() * scale)
                    && image.getPixelAt(2, 2).getAlpha() == 255,
                "Authoring shell failed scaled rendering at " + std::to_string(scale) + "x.");
    }
}

void qualifyProductShells()
{
    auto standalone = std::make_unique<drs::standalone::MainComponent>(false);
    auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*standalone, "workspaceTabs"));
    require(standaloneTabs != nullptr && standaloneTabs->getNumTabs() == 2,
            "Standalone qualification requires Perform and Map tabs.");
    standaloneTabs->setCurrentTabIndex(1);
    for (const auto size : { juce::Point<int> { 900, 700 }, { 860, 760 }, { 1120, 800 } })
    {
        standalone->setSize(size.x, size.y);
        standalone->resized();
        auto& map = requireComponent(*standalone, "authoringZoneMap");
        auto& workbench = requireComponent(*standalone, "authoringWorkbench");
        requireInside(*standalone, map, "Standalone map");
        requireInside(*standalone, workbench, "Standalone workbench");
    }

    auto processor = std::make_unique<drs::plugin::Processor>();
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
    require(editor != nullptr, "Plugin editor could not be created for shell qualification.");
    auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
    require(pluginTabs != nullptr && pluginTabs->getNumTabs() == 2,
            "Plugin qualification requires Perform and Map tabs.");
    pluginTabs->setCurrentTabIndex(1);
    for (const auto size : { juce::Point<int> { 760, 620 }, { 820, 700 }, { 900, 700 } })
    {
        editor->setSize(size.x, size.y);
        editor->resized();
        auto& map = requireComponent(*editor, "authoringZoneMap");
        auto& workbench = requireComponent(*editor, "authoringWorkbench");
        requireInside(*editor, map, "Plugin map");
        requireInside(*editor, workbench, "Plugin workbench");
        require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
                "Plugin shell matrix clipped the protected map height.");
    }
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyZoneAndGeometryMatrix();
        qualifyMapResponsiveness();
        qualifyAccessibilityAndKeyboardNavigation();
        qualifyAuthoringShellLayouts();
        qualifyProductShells();
        std::cout << "Open Workbench Phase 5 qualification passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 5 qualification failed: " << exception.what() << '\n';
        return 1;
    }
}
