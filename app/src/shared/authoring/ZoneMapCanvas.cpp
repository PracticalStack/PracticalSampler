#include "shared/authoring/ZoneMapCanvas.h"

#include <algorithm>
#include <cmath>

namespace drs::app::authoring
{
namespace
{
const auto zoneMapGrid = juce::Colour::fromRGB(230, 220, 207);
const auto zoneMapSelected = juce::Colour::fromRGB(28, 108, 88);
const auto zoneMapSecondarySelected = juce::Colour::fromRGB(71, 132, 117);
const auto zoneMapAccent = juce::Colour::fromRGB(181, 96, 21);
const auto zoneMapSelectedFill = zoneMapSelected.withAlpha(0.62f);
const auto zoneMapSecondarySelectedFill = zoneMapSecondarySelected.withAlpha(0.46f);
const auto zoneMapAccentFill = zoneMapAccent.withAlpha(0.5f);
const auto zoneMapLabelFill = juce::Colour::fromRGBA(20, 25, 31, 168);
const auto zoneMapOutline = juce::Colour::fromRGBA(24, 29, 33, 92);
const auto zoneMapFocusRing = juce::Colour::fromRGB(24, 29, 33);
const auto zoneMapFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 232);
const auto zoneMapMarqueeFill = zoneMapSelected.withAlpha(0.16f);
const auto zoneMapMarqueeOutline = zoneMapSelected.withAlpha(0.92f);
constexpr float rangeHandleRadius = 6.0f;
constexpr float rangeHandleHitRadius = 12.0f;
constexpr float marqueeDragThreshold = 4.0f;
constexpr float minimumViewportZoom = 1.0f;
constexpr float maximumViewportZoom = 8.0f;
constexpr float viewportZoomSensitivity = 1.5f;
constexpr int deleteSelectedSampleMenuItemId = 1;

bool isSupportedSampleFile(const juce::String& path)
{
    const auto extension = juce::File(path).getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".flac";
}

void drawFocusRing(juce::Graphics& g,
                   juce::Rectangle<float> bounds,
                   float cornerSize,
                   const juce::Colour& outlineColour)
{
    g.setColour(zoneMapFocusHalo);
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 3.0f);
    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerSize, 1.8f);
}
} // namespace

ZoneMapCanvas::ZoneMapCanvas()
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setColour(juce::ListBox::backgroundColourId, zoneMapGrid);
    setColour(juce::TextEditor::focusedOutlineColourId, zoneMapFocusRing);
}

void ZoneMapCanvas::setZoneSummaries(std::vector<drs::engine::AuthoringZoneSummary> summaries)
{
    zoneSummaries = std::move(summaries);
    repaint();
}

void ZoneMapCanvas::setSelectionState(SelectionState nextSelectionState)
{
    selectionState = std::move(nextSelectionState);
    repaint();
}

ZoneMapCanvas::SelectionState ZoneMapCanvas::getSelectionState() const
{
    return selectionState;
}

void ZoneMapCanvas::setOnZoneSelectionRequested(std::function<void(const std::string& zoneId)> nextCallback)
{
    onZoneSelectionRequested = std::move(nextCallback);
}

void ZoneMapCanvas::setOnZoneSelectionStateRequested(
    std::function<void(const SelectionState& selectionState)> nextCallback)
{
    onZoneSelectionStateRequested = std::move(nextCallback);
}

void ZoneMapCanvas::setOnZoneRangeCommitRequested(
    std::function<void(const std::vector<drs::engine::AuthoringZoneSummary>& zones,
                       const std::string& label)> nextCallback)
{
    onZoneRangeCommitRequested = std::move(nextCallback);
}

void ZoneMapCanvas::setOnZoneAuditionRequested(
    std::function<void(const std::string& zoneId, int midiNote, int velocity)> nextCallback)
{
    onZoneAuditionRequested = std::move(nextCallback);
}

void ZoneMapCanvas::setOnSampleFilesDropped(std::function<void(std::vector<juce::File>)> nextCallback)
{
    onSampleFilesDropped = std::move(nextCallback);
}

void ZoneMapCanvas::setOnDeleteSelectedSampleRequested(std::function<void()> nextCallback)
{
    onDeleteSelectedSampleRequested = std::move(nextCallback);
}

bool ZoneMapCanvas::isInterestedInFileDrag(const juce::StringArray& files)
{
    return onSampleFilesDropped
        && std::any_of(files.begin(), files.end(), isSupportedSampleFile);
}

void ZoneMapCanvas::fileDragEnter(const juce::StringArray& files, int, int)
{
    sampleFileDragActive = isInterestedInFileDrag(files);
    repaint();
}

void ZoneMapCanvas::fileDragExit(const juce::StringArray&)
{
    sampleFileDragActive = false;
    repaint();
}

void ZoneMapCanvas::filesDropped(const juce::StringArray& files, int, int)
{
    sampleFileDragActive = false;
    repaint();

    if (!onSampleFilesDropped)
        return;

    std::vector<juce::File> sampleFiles;
    sampleFiles.reserve(static_cast<std::size_t>(files.size()));
    for (const auto& path : files)
        if (isSupportedSampleFile(path))
            sampleFiles.emplace_back(path);

    if (!sampleFiles.empty())
        onSampleFilesDropped(std::move(sampleFiles));
}

bool ZoneMapCanvas::requestSelectionAt(juce::Point<float> position, SelectionMode mode)
{
    if (activeGesture.has_value() || activeMarqueeGesture.has_value() || activePanGesture.has_value())
        return false;

    const auto hitZoneIndex = findZoneIndexAt(position);
    if (!hitZoneIndex.has_value())
        return false;

    return requestSelectionByIndex(*hitZoneIndex, mode);
}

bool ZoneMapCanvas::requestSelectionInBounds(juce::Rectangle<float> bounds, SelectionMode mode)
{
    if (activeGesture.has_value() || activePanGesture.has_value())
        return false;

    if (bounds.isEmpty())
        return false;

    return requestSelectionState(buildSelectionStateForBounds(bounds, mode));
}

bool ZoneMapCanvas::requestAuditionAt(juce::Point<float> position)
{
    if (activeGesture.has_value() || activePanGesture.has_value() || !onZoneAuditionRequested)
        return false;

    const auto layouts = buildZoneLayouts();
    std::vector<ZoneLayout> hits;
    for (const auto& layout : layouts)
        if (layout.bounds.contains(position))
            hits.push_back(layout);
    if (hits.empty())
        return false;

    std::sort(hits.begin(), hits.end(), [](const ZoneLayout& left, const ZoneLayout& right)
    {
        return left.bounds.getWidth() * left.bounds.getHeight()
            < right.bounds.getWidth() * right.bounds.getHeight();
    });
    // Selection refreshes the authoring view model synchronously, so retain a
    // value copy across that callback instead of a reference into zoneSummaries.
    const auto zone = zoneSummaries[hits.front().index];
    if (zone.id != selectionState.primaryZoneId)
    {
        auto nextSelectionState = getSelectionState();
        if (std::find(nextSelectionState.zoneIds.begin(), nextSelectionState.zoneIds.end(), zone.id)
            == nextSelectionState.zoneIds.end())
        {
            nextSelectionState.zoneIds.push_back(zone.id);
        }
        nextSelectionState.primaryZoneId = zone.id;
        requestSelectionState(nextSelectionState);
    }
    onZoneAuditionRequested(zone.id, zone.rootKey,
                            std::clamp((zone.velocityLow + zone.velocityHigh) / 2, 1, 127));
    return true;
}

bool ZoneMapCanvas::requestDeleteSelectedSample()
{
    if (selectionState.zoneIds.empty() || !onDeleteSelectedSampleRequested)
        return false;

    onDeleteSelectedSampleRequested();
    return true;
}

bool ZoneMapCanvas::moveSelection(int direction)
{
    if (zoneSummaries.empty() || direction == 0)
        return false;

    const auto currentIndex = static_cast<int>(findSelectedZoneIndex().value_or(0));
    const auto nextIndex = juce::jlimit(0,
                                        static_cast<int>(zoneSummaries.size()) - 1,
                                        currentIndex + (direction < 0 ? -1 : 1));
    return requestSelectionByIndex(static_cast<std::size_t>(nextIndex), SelectionMode::replace);
}

bool ZoneMapCanvas::requestZoomAt(juce::Point<float> position, float wheelDelta)
{
    const auto inner = getInnerBounds();
    if (!inner.contains(position)
        || juce::approximatelyEqual(wheelDelta, 0.0f)
        || activeGesture.has_value()
        || activeMarqueeGesture.has_value()
        || activePanGesture.has_value())
    {
        return false;
    }

    const auto nextZoom = juce::jlimit(
        minimumViewportZoom,
        maximumViewportZoom,
        viewportZoom * std::exp(wheelDelta * viewportZoomSensitivity));
    if (juce::approximatelyEqual(nextZoom, viewportZoom))
        return false;

    const auto pointerProportion = juce::Point<float> {
        juce::jlimit(0.0f, 1.0f, (position.x - inner.getX()) / inner.getWidth()),
        juce::jlimit(0.0f, 1.0f, (position.y - inner.getY()) / inner.getHeight())
    };
    const auto contentAtPointer = viewportOrigin + pointerProportion / viewportZoom;
    viewportZoom = nextZoom;
    viewportOrigin = clampViewportOrigin(contentAtPointer - pointerProportion / viewportZoom);
    if (juce::approximatelyEqual(viewportZoom, minimumViewportZoom))
    {
        viewportZoom = minimumViewportZoom;
        viewportOrigin = {};
    }
    repaint();
    return true;
}

bool ZoneMapCanvas::requestPanBy(juce::Point<float> pixelDelta)
{
    if (viewportZoom <= minimumViewportZoom
        || (juce::approximatelyEqual(pixelDelta.x, 0.0f)
            && juce::approximatelyEqual(pixelDelta.y, 0.0f)))
    {
        return false;
    }

    const auto inner = getInnerBounds();
    const auto nextOrigin = clampViewportOrigin(
        viewportOrigin
        - juce::Point<float> {
            pixelDelta.x / (inner.getWidth() * viewportZoom),
            pixelDelta.y / (inner.getHeight() * viewportZoom)
        });
    if (nextOrigin == viewportOrigin)
        return false;

    viewportOrigin = nextOrigin;
    repaint();
    return true;
}

void ZoneMapCanvas::resetViewport()
{
    viewportZoom = minimumViewportZoom;
    viewportOrigin = {};
    activePanGesture.reset();
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void ZoneMapCanvas::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::transparentBlack);
    g.setColour(findColour(juce::ListBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 14.0f);

    if (hasKeyboardFocus(false))
        drawFocusRing(g, bounds.reduced(2.0f), 14.0f, findColour(juce::TextEditor::focusedOutlineColourId));

    const auto inner = getInnerBounds();
    g.setColour(juce::Colour::fromRGBA(24, 29, 33, 24));

    for (int key = 0; key <= 127; key += 12)
    {
        const auto x = normalizedContentToCanvas(
            { static_cast<float>(key) / 127.0f, 0.0f }).x;
        if (x >= inner.getX() && x <= inner.getRight())
            g.drawVerticalLine(static_cast<int>(x), inner.getY(), inner.getBottom());
    }

    for (int velocity = 1; velocity <= 127; velocity += 16)
    {
        const auto y = normalizedContentToCanvas(
            { 0.0f, 1.0f - static_cast<float>(velocity) / 127.0f }).y;
        if (y >= inner.getY() && y <= inner.getBottom())
            g.drawHorizontalLine(static_cast<int>(y), inner.getX(), inner.getRight());
    }

    const auto paintOrder = buildPaintOrder();
    const auto zoneLayouts = buildZoneLayouts();

    for (const auto zoneIndex : paintOrder)
    {
        const auto& zone = zoneSummaries[zoneIndex];
        const auto layoutIterator = std::find_if(zoneLayouts.begin(),
                                                 zoneLayouts.end(),
                                                 [&](const ZoneLayout& layout)
                                                 {
                                                     return layout.index == zoneIndex;
                                                 });
        if (layoutIterator == zoneLayouts.end())
            continue;

        const auto zoneBounds = layoutIterator->bounds;
        const auto primarySelected = selectionState.primaryZoneId == zone.id;
        const auto secondarySelected = !primarySelected
            && std::find(selectionState.zoneIds.begin(),
                         selectionState.zoneIds.end(),
                         zone.id) != selectionState.zoneIds.end();
        g.setColour(primarySelected
                        ? zoneMapSelectedFill
                        : (secondarySelected ? zoneMapSecondarySelectedFill : zoneMapAccentFill));
        g.fillRoundedRectangle(zoneBounds, 8.0f);

        g.setColour(primarySelected
                        ? juce::Colours::white
                        : (secondarySelected ? zoneMapSecondarySelected : zoneMapOutline));
        g.drawRoundedRectangle(zoneBounds.reduced(0.75f), 8.0f, primarySelected ? 2.0f : 1.2f);

        auto labelBounds = zoneBounds.toNearestInt().reduced(6, 4);
        labelBounds.setWidth(std::min(labelBounds.getWidth(), 132));
        labelBounds.setHeight(std::min(labelBounds.getHeight(), 20));
        g.setColour(primarySelected
                        ? zoneMapLabelFill.brighter(0.12f)
                        : (secondarySelected ? zoneMapLabelFill.brighter(0.04f) : zoneMapLabelFill));
        g.fillRoundedRectangle(labelBounds.toFloat(), 5.0f);

        g.setColour(juce::Colours::white.withAlpha(primarySelected || secondarySelected ? 1.0f : 0.92f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(zone.displayName.c_str()),
                         labelBounds.reduced(6, 2),
                         juce::Justification::centredLeft,
                         1);

        if (primarySelected)
        {
            const auto handleCenters = buildHandleCenters(computeZoneBounds(getDisplayZoneSummary(zoneIndex)));
            for (const auto& [handle, center] : handleCenters)
            {
                if (!inner.contains(center))
                    continue;
                const auto activeHandle = activeGesture.has_value()
                    && activeGesture->zoneIndex == zoneIndex
                    && activeGesture->handle == handle;
                g.setColour(activeHandle ? findColour(juce::TextEditor::focusedOutlineColourId) : juce::Colours::white);
                g.fillEllipse(center.x - rangeHandleRadius,
                              center.y - rangeHandleRadius,
                              rangeHandleRadius * 2.0f,
                              rangeHandleRadius * 2.0f);
                g.setColour(zoneMapOutline.withAlpha(activeHandle ? 1.0f : 0.85f));
                g.drawEllipse(center.x - rangeHandleRadius,
                              center.y - rangeHandleRadius,
                              rangeHandleRadius * 2.0f,
                              rangeHandleRadius * 2.0f,
                              activeHandle ? 2.0f : 1.0f);
            }
        }
    }

    if (sampleFileDragActive)
    {
        const auto dropBounds = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(zoneMapSelected.withAlpha(0.16f));
        g.fillRoundedRectangle(dropBounds, 12.0f);
        g.setColour(zoneMapSelected);
        g.drawRoundedRectangle(dropBounds, 12.0f, 3.0f);
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawFittedText("Drop WAV or FLAC files to import",
                         dropBounds.toNearestInt().reduced(16),
                         juce::Justification::centred,
                         1);
    }

    if (activeMarqueeGesture.has_value() && activeMarqueeGesture->dragged)
    {
        const auto marqueeBounds = juce::Rectangle<float>(activeMarqueeGesture->start,
                                                          activeMarqueeGesture->current).getSmallestIntegerContainer().toFloat();
        g.setColour(zoneMapMarqueeFill);
        g.fillRoundedRectangle(marqueeBounds, 6.0f);
        g.setColour(zoneMapMarqueeOutline);
        g.drawRoundedRectangle(marqueeBounds, 6.0f, 1.5f);
    }

    const auto zoomText = viewportZoom > minimumViewportZoom
        ? juce::String(std::lround(viewportZoom * 100.0f)) + "%  |  Drag empty space to pan"
        : juce::String("Ctrl + scroll to zoom");
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    const auto zoomTextWidth = std::min(getWidth() - 24,
                                        viewportZoom > minimumViewportZoom ? 220 : 142);
    auto zoomBounds = getLocalBounds().reduced(12).removeFromTop(22).removeFromRight(zoomTextWidth);
    g.setColour(zoneMapLabelFill);
    g.fillRoundedRectangle(zoomBounds.toFloat(), 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawFittedText(zoomText, zoomBounds.reduced(8, 2), juce::Justification::centredRight, 1);
}

void ZoneMapCanvas::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    if (event.mods.isPopupMenu())
    {
        cancelActiveRangeGesture();
        if (const auto hitZoneIndex = findZoneIndexAt(event.position); hitZoneIndex.has_value())
        {
            const auto& hitZoneId = zoneSummaries[*hitZoneIndex].id;
            if (std::find(selectionState.zoneIds.begin(),
                          selectionState.zoneIds.end(),
                          hitZoneId) == selectionState.zoneIds.end())
            {
                requestSelectionByIndex(*hitZoneIndex, SelectionMode::replace);
            }
        }
        showContextMenuAt(event.getScreenPosition());
        return;
    }

    if (beginRangeGestureAt(event.position))
        return;

    if (viewportZoom > minimumViewportZoom
        && event.mods.isLeftButtonDown()
        && !findZoneIndexAt(event.position).has_value())
    {
        activePanGesture = PanGesture { event.position, viewportOrigin };
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }

    activeMarqueeGesture = MarqueeGesture { event.position, event.position, event.mods.isCtrlDown(), false };
}

void ZoneMapCanvas::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        return;

    grabKeyboardFocus();
    requestAuditionAt(event.position);
}

void ZoneMapCanvas::showContextMenuAt(juce::Point<int> screenPosition)
{
    const auto selectionCount = selectionState.zoneIds.size();
    juce::PopupMenu menu;
    menu.addItem(deleteSelectedSampleMenuItemId,
                 selectionCount > 1 ? "Delete Selected Zones" : "Delete Selected Sample",
                 selectionCount > 0 && onDeleteSelectedSampleRequested != nullptr);

    auto safeThis = juce::Component::SafePointer<ZoneMapCanvas>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                           {screenPosition.x, screenPosition.y, 1, 1}),
                       [safeThis](int menuItemId)
                       {
                           if (safeThis != nullptr && menuItemId == deleteSelectedSampleMenuItemId)
                               safeThis->requestDeleteSelectedSample();
                       });
}

void ZoneMapCanvas::mouseDrag(const juce::MouseEvent& event)
{
    if (activeGesture.has_value())
    {
        updateActiveRangeGesture(event.position);
        return;
    }

    if (activePanGesture.has_value())
    {
        const auto inner = getInnerBounds();
        const auto pixelDelta = event.position - activePanGesture->start;
        viewportOrigin = clampViewportOrigin(
            activePanGesture->initialViewportOrigin
            - juce::Point<float> {
                pixelDelta.x / (inner.getWidth() * viewportZoom),
                pixelDelta.y / (inner.getHeight() * viewportZoom)
            });
        repaint();
        return;
    }

    if (!activeMarqueeGesture.has_value())
        return;

    activeMarqueeGesture->current = event.position;
    if (!activeMarqueeGesture->dragged
        && activeMarqueeGesture->start.getDistanceFrom(activeMarqueeGesture->current) >= marqueeDragThreshold)
    {
        activeMarqueeGesture->dragged = true;
    }
    repaint();
}

void ZoneMapCanvas::mouseUp(const juce::MouseEvent& event)
{
    if (activeGesture.has_value())
    {
        endActiveRangeGesture(event.position);
        return;
    }

    if (activePanGesture.has_value())
    {
        activePanGesture.reset();
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
        return;
    }

    if (!activeMarqueeGesture.has_value())
        return;

    const auto gesture = *activeMarqueeGesture;
    activeMarqueeGesture.reset();
    repaint();

    if (gesture.dragged)
    {
        const auto marqueeBounds = juce::Rectangle<float>(gesture.start, gesture.current);
        requestSelectionInBounds(marqueeBounds,
                                 gesture.ctrlDown ? SelectionMode::additive : SelectionMode::replace);
        return;
    }

    requestSelectionAt(event.position,
                       gesture.ctrlDown ? SelectionMode::toggle : SelectionMode::replace);
}

void ZoneMapCanvas::mouseWheelMove(const juce::MouseEvent& event,
                                   const juce::MouseWheelDetails& wheel)
{
    if (!event.mods.isCtrlDown())
    {
        juce::Component::mouseWheelMove(event, wheel);
        return;
    }

    const auto wheelDelta = !juce::approximatelyEqual(wheel.deltaY, 0.0f)
        ? wheel.deltaY
        : wheel.deltaX;
    requestZoomAt(event.position, wheelDelta);
}

bool ZoneMapCanvas::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && activeGesture.has_value())
        return cancelActiveRangeGesture();

    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::upKey)
        return moveSelection(-1);

    if (key == juce::KeyPress::rightKey || key == juce::KeyPress::downKey)
        return moveSelection(1);

    if (key == juce::KeyPress::homeKey)
        return requestSelectionByIndex(0);

    if (key == juce::KeyPress::endKey && !zoneSummaries.empty())
        return requestSelectionByIndex(zoneSummaries.size() - 1);

    return false;
}

void ZoneMapCanvas::focusGained(FocusChangeType)
{
    repaint();
}

void ZoneMapCanvas::focusLost(FocusChangeType)
{
    repaint();
}

juce::Rectangle<float> ZoneMapCanvas::getInnerBounds() const
{
    return getLocalBounds().toFloat().reduced(12.0f);
}

juce::Point<float> ZoneMapCanvas::clampViewportOrigin(juce::Point<float> origin) const
{
    const auto maximumOrigin = 1.0f - 1.0f / viewportZoom;
    return {
        juce::jlimit(0.0f, maximumOrigin, origin.x),
        juce::jlimit(0.0f, maximumOrigin, origin.y)
    };
}

juce::Point<float> ZoneMapCanvas::normalizedContentToCanvas(juce::Point<float> position) const
{
    const auto inner = getInnerBounds();
    return {
        inner.getX() + (position.x - viewportOrigin.x) * viewportZoom * inner.getWidth(),
        inner.getY() + (position.y - viewportOrigin.y) * viewportZoom * inner.getHeight()
    };
}

drs::engine::AuthoringZoneSummary ZoneMapCanvas::getDisplayZoneSummary(std::size_t index) const
{
    if (activeGesture.has_value())
    {
        const auto iterator = std::find(activeGesture->zoneIndices.begin(),
                                        activeGesture->zoneIndices.end(),
                                        index);
        if (iterator != activeGesture->zoneIndices.end())
        {
            const auto previewIndex = static_cast<std::size_t>(
                std::distance(activeGesture->zoneIndices.begin(), iterator));
            return activeGesture->previewZones[previewIndex];
        }
    }

    return zoneSummaries[index];
}

juce::Rectangle<float> ZoneMapCanvas::computeZoneBounds(const drs::engine::AuthoringZoneSummary& zone) const
{
    const auto inner = getInnerBounds();
    const auto topLeft = normalizedContentToCanvas({
        static_cast<float>(zone.keyLow) / 127.0f,
        1.0f - static_cast<float>(zone.velocityHigh) / 127.0f
    });
    const auto x = topLeft.x;
    const auto width = std::max(10.0f,
                                inner.getWidth() * viewportZoom
                                    * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityHeight = static_cast<float>(zone.velocityHigh - zone.velocityLow) / 127.0f;
    const auto height = std::max(14.0f, inner.getHeight() * viewportZoom * normalizedVelocityHeight);
    return {x, topLeft.y, width, height};
}

std::vector<ZoneMapCanvas::ZoneLayout> ZoneMapCanvas::buildZoneLayouts() const
{
    std::vector<ZoneLayout> layouts;
    layouts.reserve(zoneSummaries.size());

    const auto inner = getInnerBounds();
    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        const auto visibleBounds = computeZoneBounds(getDisplayZoneSummary(index)).getIntersection(inner);
        if (!visibleBounds.isEmpty())
            layouts.push_back({index, visibleBounds});
    }

    return layouts;
}

std::vector<std::size_t> ZoneMapCanvas::buildPaintOrder() const
{
    std::vector<std::size_t> order;
    order.reserve(zoneSummaries.size());

    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        if (!zoneSummaries[index].selected && !zoneSummaries[index].additionallySelected)
            order.push_back(index);
    }

    for (const auto selectedIndex : findSecondarySelectedZoneIndices())
        order.push_back(selectedIndex);

    if (const auto selectedIndex = findSelectedZoneIndex(); selectedIndex.has_value())
        order.push_back(*selectedIndex);

    return order;
}

std::optional<std::size_t> ZoneMapCanvas::findZoneIndexAt(juce::Point<float> position) const
{
    const auto zoneLayouts = buildZoneLayouts();
    std::vector<ZoneLayout> hits;

    for (const auto& layout : zoneLayouts)
    {
        if (layout.bounds.contains(position))
            hits.push_back(layout);
    }

    if (hits.empty())
        return std::nullopt;

    std::sort(hits.begin(),
              hits.end(),
              [&](const ZoneLayout& left, const ZoneLayout& right)
              {
                  const auto leftArea = left.bounds.getWidth() * left.bounds.getHeight();
                  const auto rightArea = right.bounds.getWidth() * right.bounds.getHeight();

                  if (!juce::approximatelyEqual(leftArea, rightArea))
                      return leftArea < rightArea;

                  return zoneSummaries[left.index].id == selectionState.primaryZoneId
                      && zoneSummaries[right.index].id != selectionState.primaryZoneId;
              });

    return hits.front().index;
}

std::optional<std::size_t> ZoneMapCanvas::findSelectedZoneIndex() const
{
    if (!selectionState.primaryZoneId.empty())
    {
        const auto iterator = std::find_if(zoneSummaries.begin(),
                                           zoneSummaries.end(),
                                           [&](const auto& zone)
                                           {
                                               return zone.id == selectionState.primaryZoneId;
                                           });
        if (iterator != zoneSummaries.end())
            return static_cast<std::size_t>(std::distance(zoneSummaries.begin(), iterator));
    }

    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        if (getDisplayZoneSummary(index).selected)
            return index;
    }

    return std::nullopt;
}

std::vector<std::size_t> ZoneMapCanvas::findSecondarySelectedZoneIndices() const
{
    std::vector<std::size_t> indices;
    if (!selectionState.zoneIds.empty())
    {
        for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
        {
            const auto& zoneId = zoneSummaries[index].id;
            if (zoneId == selectionState.primaryZoneId)
                continue;

            if (std::find(selectionState.zoneIds.begin(),
                          selectionState.zoneIds.end(),
                          zoneId) != selectionState.zoneIds.end())
            {
                indices.push_back(index);
            }
        }

        return indices;
    }

    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        if (getDisplayZoneSummary(index).additionallySelected)
            indices.push_back(index);
    }

    return indices;
}

std::vector<std::pair<ZoneMapCanvas::RangeHandle, juce::Point<float>>>
ZoneMapCanvas::buildHandleCenters(const juce::Rectangle<float>& zoneBounds) const
{
    return {
        {RangeHandle::keyLow, {zoneBounds.getX(), zoneBounds.getCentreY()}},
        {RangeHandle::keyHigh, {zoneBounds.getRight(), zoneBounds.getCentreY()}},
        {RangeHandle::velocityHigh, {zoneBounds.getCentreX(), zoneBounds.getY()}},
        {RangeHandle::velocityLow, {zoneBounds.getCentreX(), zoneBounds.getBottom()}}
    };
}

ZoneMapCanvas::RangeHandle ZoneMapCanvas::findRangeHandleAt(juce::Point<float> position, std::size_t& zoneIndex) const
{
    const auto selectedIndex = findSelectedZoneIndex();
    if (!selectedIndex.has_value())
        return RangeHandle::none;

    const auto zoneBounds = computeZoneBounds(getDisplayZoneSummary(*selectedIndex));
    const auto handleCenters = buildHandleCenters(zoneBounds);
    auto nearestHandle = RangeHandle::none;
    auto nearestDistance = rangeHandleHitRadius;
    const auto inner = getInnerBounds();

    for (const auto& [handle, center] : handleCenters)
    {
        if (!inner.contains(center))
            continue;
        const auto distance = center.getDistanceFrom(position);
        if (distance <= nearestDistance)
        {
            nearestDistance = distance;
            nearestHandle = handle;
        }
    }

    if (nearestHandle != RangeHandle::none)
        zoneIndex = *selectedIndex;

    return nearestHandle;
}

std::vector<drs::engine::AuthoringZoneSummary> ZoneMapCanvas::buildRangePreviews(
    const RangeGesture& gesture,
    juce::Point<float> position) const
{
    if (gesture.originalZones.empty())
        return {};

    auto previews = gesture.originalZones;
    auto primaryPreview = gesture.originalZones.front();

    if (gesture.handle == RangeHandle::keyLow || gesture.handle == RangeHandle::keyHigh)
    {
        const auto primaryLowBoundary = primaryPreview.keyLow;
        const auto primaryHighBoundary = primaryPreview.keyHigh + 1;
        const auto anchor = gesture.handle == RangeHandle::keyHigh
            ? primaryLowBoundary : primaryHighBoundary;
        const auto targetKey = positionToMidiKey(position);
        const auto requestedPrimaryWidth = gesture.handle == RangeHandle::keyHigh
            ? juce::jlimit(1, 128 - primaryLowBoundary, targetKey - primaryLowBoundary + 1)
            : juce::jlimit(1, primaryHighBoundary, primaryHighBoundary - targetKey);
        const auto originalPrimaryWidth = primaryHighBoundary - primaryLowBoundary;
        auto scale = static_cast<double>(requestedPrimaryWidth)
            / static_cast<double>(originalPrimaryWidth);

        auto minimumBoundary = 128;
        auto maximumBoundary = 0;
        auto minimumScale = 0.0;
        for (const auto& zone : gesture.originalZones)
        {
            minimumBoundary = std::min(minimumBoundary, zone.keyLow);
            maximumBoundary = std::max(maximumBoundary, zone.keyHigh + 1);
            minimumScale = std::max(minimumScale,
                                    1.0 / static_cast<double>(zone.keyHigh - zone.keyLow + 1));
        }

        auto maximumScale = 128.0;
        if (minimumBoundary < anchor)
        {
            maximumScale = std::min(maximumScale,
                                    static_cast<double>(anchor)
                                        / static_cast<double>(anchor - minimumBoundary));
        }
        if (maximumBoundary > anchor)
        {
            maximumScale = std::min(maximumScale,
                                    static_cast<double>(128 - anchor)
                                        / static_cast<double>(maximumBoundary - anchor));
        }
        scale = juce::jlimit(minimumScale, maximumScale, scale);

        const auto scaleBoundary = [anchor, scale](int boundary)
        {
            return juce::jlimit(0, 128,
                                static_cast<int>(std::lround(
                                    static_cast<double>(anchor)
                                    + static_cast<double>(boundary - anchor) * scale)));
        };
        for (auto& preview : previews)
        {
            auto scaledLow = scaleBoundary(preview.keyLow);
            auto scaledHighBoundary = scaleBoundary(preview.keyHigh + 1);
            if (scaledHighBoundary <= scaledLow)
                scaledHighBoundary = std::min(128, scaledLow + 1);
            if (scaledHighBoundary <= scaledLow)
                scaledLow = std::max(0, scaledHighBoundary - 1);
            preview.keyLow = scaledLow;
            preview.keyHigh = scaledHighBoundary - 1;
        }
        return previews;
    }

    switch (gesture.handle)
    {
        case RangeHandle::velocityHigh:
            primaryPreview.velocityHigh = juce::jlimit(primaryPreview.velocityLow, 127,
                                                       positionToMidiVelocity(position));
            break;
        case RangeHandle::velocityLow:
            primaryPreview.velocityLow = juce::jlimit(1, primaryPreview.velocityHigh,
                                                      positionToMidiVelocity(position));
            break;
        case RangeHandle::keyLow:
        case RangeHandle::keyHigh:
        case RangeHandle::none:
            break;
    }

    const auto& primaryOriginal = gesture.originalZones.front();
    const auto velocityLowDelta = primaryPreview.velocityLow - primaryOriginal.velocityLow;
    const auto velocityHighDelta = primaryPreview.velocityHigh - primaryOriginal.velocityHigh;

    for (auto& preview : previews)
    {
        switch (gesture.handle)
        {
            case RangeHandle::velocityHigh:
                preview.velocityHigh = juce::jlimit(preview.velocityLow, 127,
                                                    preview.velocityHigh + velocityHighDelta);
                break;
            case RangeHandle::velocityLow:
                preview.velocityLow = juce::jlimit(1, preview.velocityHigh,
                                                   preview.velocityLow + velocityLowDelta);
                break;
            case RangeHandle::keyLow:
            case RangeHandle::keyHigh:
            case RangeHandle::none:
                break;
        }
    }

    return previews;
}

int ZoneMapCanvas::positionToMidiKey(juce::Point<float> position) const
{
    const auto inner = getInnerBounds();
    const auto viewportProportion = juce::jlimit(
        0.0f, 1.0f, (position.x - inner.getX()) / inner.getWidth());
    const auto contentProportion = viewportOrigin.x + viewportProportion / viewportZoom;
    return juce::jlimit(0, 127, static_cast<int>(std::lround(contentProportion * 127.0f)));
}

int ZoneMapCanvas::positionToMidiVelocity(juce::Point<float> position) const
{
    const auto inner = getInnerBounds();
    const auto viewportProportion = juce::jlimit(
        0.0f, 1.0f, (position.y - inner.getY()) / inner.getHeight());
    const auto contentProportion = viewportOrigin.y + viewportProportion / viewportZoom;
    return juce::jlimit(1, 127, static_cast<int>(std::lround((1.0f - contentProportion) * 127.0f)));
}

ZoneMapCanvas::SelectionState ZoneMapCanvas::buildSelectionStateForZoneIndex(std::size_t index,
                                                                             SelectionMode mode) const
{
    SelectionState nextSelectionState = getSelectionState();
    if (index >= zoneSummaries.size())
        return nextSelectionState;

    const auto& zoneId = zoneSummaries[index].id;
    const auto selectedIterator = std::find(nextSelectionState.zoneIds.begin(),
                                            nextSelectionState.zoneIds.end(),
                                            zoneId);
    const auto alreadySelected = selectedIterator != nextSelectionState.zoneIds.end();

    switch (mode)
    {
        case SelectionMode::replace:
            nextSelectionState.zoneIds = { zoneId };
            nextSelectionState.primaryZoneId = zoneId;
            break;
        case SelectionMode::toggle:
            if (alreadySelected)
            {
                if (nextSelectionState.zoneIds.size() == 1)
                    break;

                nextSelectionState.zoneIds.erase(selectedIterator);
                if (nextSelectionState.primaryZoneId == zoneId)
                    nextSelectionState.primaryZoneId = nextSelectionState.zoneIds.front();
            }
            else
            {
                nextSelectionState.zoneIds.push_back(zoneId);
                nextSelectionState.primaryZoneId = zoneId;
            }
            break;
        case SelectionMode::additive:
            if (!alreadySelected)
                nextSelectionState.zoneIds.push_back(zoneId);
            if (nextSelectionState.primaryZoneId.empty())
                nextSelectionState.primaryZoneId = zoneId;
            break;
    }

    return nextSelectionState;
}

ZoneMapCanvas::SelectionState ZoneMapCanvas::buildSelectionStateForBounds(juce::Rectangle<float> bounds,
                                                                          SelectionMode mode) const
{
    SelectionState nextSelectionState = getSelectionState();
    std::vector<std::string> hitZoneIds;
    const auto layouts = buildZoneLayouts();

    for (const auto& layout : layouts)
    {
        if (layout.bounds.intersects(bounds))
            hitZoneIds.push_back(zoneSummaries[layout.index].id);
    }

    if (hitZoneIds.empty())
        return nextSelectionState;

    switch (mode)
    {
        case SelectionMode::replace:
            nextSelectionState.zoneIds = hitZoneIds;
            if (nextSelectionState.primaryZoneId.empty()
                || std::find(hitZoneIds.begin(), hitZoneIds.end(), nextSelectionState.primaryZoneId)
                    == hitZoneIds.end())
            {
                nextSelectionState.primaryZoneId = hitZoneIds.front();
            }
            break;
        case SelectionMode::additive:
        {
            for (const auto& zoneId : hitZoneIds)
            {
                if (std::find(nextSelectionState.zoneIds.begin(),
                              nextSelectionState.zoneIds.end(),
                              zoneId) == nextSelectionState.zoneIds.end())
                {
                    nextSelectionState.zoneIds.push_back(zoneId);
                }
            }
            if (nextSelectionState.primaryZoneId.empty())
                nextSelectionState.primaryZoneId = hitZoneIds.front();
            break;
        }
        case SelectionMode::toggle:
            for (const auto& zoneId : hitZoneIds)
            {
                const auto iterator = std::find(nextSelectionState.zoneIds.begin(),
                                                nextSelectionState.zoneIds.end(),
                                                zoneId);
                if (iterator == nextSelectionState.zoneIds.end())
                {
                    nextSelectionState.zoneIds.push_back(zoneId);
                    continue;
                }

                if (nextSelectionState.zoneIds.size() == 1)
                    continue;

                nextSelectionState.zoneIds.erase(iterator);
                if (nextSelectionState.primaryZoneId == zoneId && !nextSelectionState.zoneIds.empty())
                    nextSelectionState.primaryZoneId = nextSelectionState.zoneIds.front();
            }
            if (nextSelectionState.primaryZoneId.empty() && !nextSelectionState.zoneIds.empty())
                nextSelectionState.primaryZoneId = nextSelectionState.zoneIds.front();
            break;
    }

    return nextSelectionState;
}

bool ZoneMapCanvas::requestSelectionByIndex(std::size_t index, SelectionMode mode)
{
    if (activeGesture.has_value() || activePanGesture.has_value())
        return false;

    if (index >= zoneSummaries.size())
        return false;

    return requestSelectionState(buildSelectionStateForZoneIndex(index, mode));
}

bool ZoneMapCanvas::requestSelectionState(const SelectionState& nextSelectionState)
{
    if (nextSelectionState.zoneIds.empty() || nextSelectionState.primaryZoneId.empty())
        return false;

    if (onZoneSelectionStateRequested)
    {
        onZoneSelectionStateRequested(nextSelectionState);
        return true;
    }

    if (onZoneSelectionRequested && nextSelectionState.primaryZoneId != selectionState.primaryZoneId)
    {
        onZoneSelectionRequested(nextSelectionState.primaryZoneId);
        return true;
    }

    return false;
}

bool ZoneMapCanvas::beginRangeGestureAt(juce::Point<float> position)
{
    if (activeGesture.has_value())
        return false;

    std::size_t zoneIndex = 0;
    const auto handle = findRangeHandleAt(position, zoneIndex);
    if (handle == RangeHandle::none)
        return false;

    RangeGesture gesture;
    gesture.handle = handle;
    gesture.zoneIndex = zoneIndex;
    gesture.zoneIndices.push_back(zoneIndex);
    for (const auto secondaryIndex : findSecondarySelectedZoneIndices())
    {
        if (secondaryIndex != zoneIndex)
            gesture.zoneIndices.push_back(secondaryIndex);
    }
    gesture.originalZones.reserve(gesture.zoneIndices.size());
    for (const auto selectedZoneIndex : gesture.zoneIndices)
        gesture.originalZones.push_back(zoneSummaries[selectedZoneIndex]);
    gesture.previewZones = gesture.originalZones;
    activeGesture = gesture;
    repaint();
    return true;
}

bool ZoneMapCanvas::updateActiveRangeGesture(juce::Point<float> position)
{
    if (!activeGesture.has_value())
        return false;

    activeGesture->previewZones = buildRangePreviews(*activeGesture, position);
    repaint();
    return true;
}

bool ZoneMapCanvas::endActiveRangeGesture(juce::Point<float> position)
{
    if (!activeGesture.has_value())
        return false;

    activeGesture->previewZones = buildRangePreviews(*activeGesture, position);
    const auto changed = !std::equal(activeGesture->previewZones.begin(),
                                     activeGesture->previewZones.end(),
                                     activeGesture->originalZones.begin(),
                                     [](const auto& preview, const auto& original)
                                     {
                                         return preview.keyLow == original.keyLow
                                             && preview.keyHigh == original.keyHigh
                                             && preview.velocityLow == original.velocityLow
                                             && preview.velocityHigh == original.velocityHigh;
                                     });

    auto committedZones = activeGesture->previewZones;
    const auto label = (activeGesture->handle == RangeHandle::keyLow || activeGesture->handle == RangeHandle::keyHigh)
        ? std::string("Update zone key range")
        : std::string("Update zone velocity range");
    activeGesture.reset();
    repaint();

    if (changed && onZoneRangeCommitRequested)
        onZoneRangeCommitRequested(committedZones, label);

    return true;
}

bool ZoneMapCanvas::cancelActiveRangeGesture()
{
    if (!activeGesture.has_value())
        return false;

    activeGesture.reset();
    repaint();
    return true;
}
} // namespace drs::app::authoring
