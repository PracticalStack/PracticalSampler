#include "shared/authoring/ZoneMapOverview.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace drs::app::authoring
{
namespace
{
const auto overviewSurface = visual::mapSurface;
const auto overviewBorder = visual::borderStrong;
const auto selectionOrange = visual::selection;
}

ZoneMapOverview::ZoneMapOverview()
{
    setComponentID("authoringZoneMapMinimap");
    setTitle("Zone Map overview");
    setDescription("Full key and velocity map. Click or drag the viewport frame to navigate.");
    setHelpText("Use the arrow keys to move the visible map region without changing selection.");
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ZoneMapOverview::setZones(std::vector<Zone> nextZones)
{
    zones = std::move(nextZones);
    groups.clear();
    std::unordered_map<std::string, std::size_t> groupIndexById;
    groupIndexById.reserve(zones.size());
    for (const auto& zone : zones)
    {
        const auto groupId = zone.groupId.empty() ? std::string { "ungrouped" } : zone.groupId;
        const auto [iterator, inserted] = groupIndexById.emplace(groupId, groups.size());
        if (inserted)
            groups.push_back({ groupId, zone.normalizedBounds });
        else
            groups[iterator->second].normalizedBounds =
                groups[iterator->second].normalizedBounds.getUnion(zone.normalizedBounds);
    }
    repaint();
}

void ZoneMapOverview::setSelection(std::vector<std::string> zoneIds)
{
    selectedZoneIds = std::move(zoneIds);
    repaint();
}

void ZoneMapOverview::setViewport(juce::Rectangle<float> normalizedViewport)
{
    viewport = normalizedViewport.getIntersection({ 0.0f, 0.0f, 1.0f, 1.0f });
    repaint();
}

void ZoneMapOverview::setOnViewportOriginRequested(
    std::function<void(juce::Point<float>)> callback)
{
    onViewportOriginRequested = std::move(callback);
}

juce::Rectangle<float> ZoneMapOverview::getPlotBounds() const
{
    return getLocalBounds().toFloat().reduced(6.0f);
}

juce::Rectangle<float> ZoneMapOverview::contentToComponent(juce::Rectangle<float> content) const
{
    const auto plot = getPlotBounds();
    return { plot.getX() + content.getX() * plot.getWidth(),
             plot.getY() + content.getY() * plot.getHeight(),
             content.getWidth() * plot.getWidth(),
             content.getHeight() * plot.getHeight() };
}

juce::Point<float> ZoneMapOverview::componentToContent(juce::Point<float> position) const
{
    const auto plot = getPlotBounds();
    return { juce::jlimit(0.0f, 1.0f, (position.x - plot.getX()) / plot.getWidth()),
             juce::jlimit(0.0f, 1.0f, (position.y - plot.getY()) / plot.getHeight()) };
}

juce::Rectangle<float> ZoneMapOverview::getViewportFrameBounds() const
{
    return contentToComponent(viewport);
}

void ZoneMapOverview::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot = getPlotBounds();
    g.setColour(overviewSurface.withAlpha(0.97f));
    g.fillRect(bounds);
    g.setColour(overviewBorder.withAlpha(0.28f));
    g.drawRect(bounds.reduced(0.5f), 1.0f);

    for (const auto& group : groups)
    {
        const auto groupBounds = contentToComponent(group.normalizedBounds).getIntersection(plot);
        g.setColour(stableZoneGroupTint(group.id).withAlpha(0.48f));
        g.fillRect(groupBounds);
        g.setColour(overviewBorder.withAlpha(0.28f));
        g.drawRect(groupBounds, 0.75f);
    }

    std::unordered_set<std::string> selection(selectedZoneIds.begin(), selectedZoneIds.end());
    for (const auto& zone : zones)
    {
        if (selection.count(zone.id) == 0u)
            continue;
        auto selectedBounds = contentToComponent(zone.normalizedBounds).getIntersection(plot);
        if (selectedBounds.getWidth() < 2.0f)
            selectedBounds = selectedBounds.withSizeKeepingCentre(2.0f, selectedBounds.getHeight());
        if (selectedBounds.getHeight() < 2.0f)
            selectedBounds = selectedBounds.withSizeKeepingCentre(selectedBounds.getWidth(), 2.0f);
        g.setColour(selectionOrange.withAlpha(0.9f));
        g.drawRect(selectedBounds, 1.5f);
    }

    const auto frame = getViewportFrameBounds();
    g.setColour(visual::surfaceRaised.withAlpha(0.96f));
    g.drawRect(frame.expanded(1.0f), 3.0f);
    g.setColour(overviewBorder);
    g.drawRect(frame, 1.5f);

    if (hasKeyboardFocus(false))
        visual::drawFocusRing(g, bounds.reduced(2.0f), visual::controlRadius);
}

void ZoneMapOverview::requestViewportAt(const juce::Point<float> contentPosition)
{
    if (!onViewportOriginRequested)
        return;
    const auto maximumOrigin = juce::Point<float> {
        std::max(0.0f, 1.0f - viewport.getWidth()),
        std::max(0.0f, 1.0f - viewport.getHeight())
    };
    onViewportOriginRequested({ juce::jlimit(0.0f, maximumOrigin.x, contentPosition.x - dragOffset.x),
                                juce::jlimit(0.0f, maximumOrigin.y, contentPosition.y - dragOffset.y) });
}

void ZoneMapOverview::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    const auto contentPosition = componentToContent(event.position);
    const auto frame = getViewportFrameBounds();
    const auto hitFrame = frame.withSizeKeepingCentre(std::max(6.0f, frame.getWidth()),
                                                       std::max(6.0f, frame.getHeight()));
    dragOffset = hitFrame.contains(event.position)
        ? contentPosition - viewport.getPosition()
        : juce::Point<float> { viewport.getWidth() * 0.5f, viewport.getHeight() * 0.5f };
    draggingViewport = true;
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    requestViewportAt(contentPosition);
}

void ZoneMapOverview::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingViewport)
        requestViewportAt(componentToContent(event.position));
}

void ZoneMapOverview::mouseUp(const juce::MouseEvent&)
{
    draggingViewport = false;
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

bool ZoneMapOverview::keyPressed(const juce::KeyPress& key)
{
    auto delta = juce::Point<float> {};
    const auto stepX = std::max(0.01f, viewport.getWidth() * (key.getModifiers().isShiftDown() ? 0.25f : 0.08f));
    const auto stepY = std::max(0.01f, viewport.getHeight() * (key.getModifiers().isShiftDown() ? 0.25f : 0.08f));
    if (key == juce::KeyPress::leftKey)
        delta.x = -stepX;
    else if (key == juce::KeyPress::rightKey)
        delta.x = stepX;
    else if (key == juce::KeyPress::upKey)
        delta.y = -stepY;
    else if (key == juce::KeyPress::downKey)
        delta.y = stepY;
    else
        return false;

    dragOffset = { viewport.getWidth() * 0.5f, viewport.getHeight() * 0.5f };
    requestViewportAt(viewport.getCentre() + delta);
    return true;
}

void ZoneMapOverview::focusGained(FocusChangeType)
{
    repaint();
}

void ZoneMapOverview::focusLost(FocusChangeType)
{
    repaint();
}
} // namespace drs::app::authoring
