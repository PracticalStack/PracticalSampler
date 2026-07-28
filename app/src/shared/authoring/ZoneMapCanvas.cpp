#include "shared/authoring/ZoneMapCanvas.h"

#include <algorithm>
#include <cmath>

namespace drs::app::authoring
{
namespace
{
const auto zoneMapGrid = juce::Colour::fromRGB(230, 220, 207);
const auto zoneMapSelected = juce::Colour::fromRGB(28, 108, 88);
const auto zoneMapAccent = juce::Colour::fromRGB(181, 96, 21);
const auto zoneMapSelectedFill = zoneMapSelected.withAlpha(0.62f);
const auto zoneMapAccentFill = zoneMapAccent.withAlpha(0.5f);
const auto zoneMapLabelFill = juce::Colour::fromRGBA(20, 25, 31, 168);
const auto zoneMapOutline = juce::Colour::fromRGBA(24, 29, 33, 92);
const auto zoneMapFocusRing = juce::Colour::fromRGB(24, 29, 33);
const auto zoneMapFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 232);
constexpr float rangeHandleRadius = 6.0f;
constexpr float rangeHandleHitRadius = 12.0f;
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

void ZoneMapCanvas::setOnZoneSelectionRequested(std::function<void(const std::string& zoneId)> nextCallback)
{
    onZoneSelectionRequested = std::move(nextCallback);
}

void ZoneMapCanvas::setOnZoneRangeCommitRequested(
    std::function<void(const drs::engine::AuthoringZoneSummary& zone, const std::string& label)> nextCallback)
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

bool ZoneMapCanvas::requestSelectionAt(juce::Point<float> position)
{
    if (activeGesture.has_value())
        return false;

    const auto zoneLayouts = buildZoneLayouts();
    std::vector<ZoneLayout> hits;

    for (const auto& layout : zoneLayouts)
    {
        if (layout.bounds.contains(position))
            hits.push_back(layout);
    }

    if (hits.empty())
        return false;

    std::sort(hits.begin(),
              hits.end(),
              [&](const ZoneLayout& left, const ZoneLayout& right)
              {
                  const auto leftArea = left.bounds.getWidth() * left.bounds.getHeight();
                  const auto rightArea = right.bounds.getWidth() * right.bounds.getHeight();

                  if (!juce::approximatelyEqual(leftArea, rightArea))
                      return leftArea < rightArea;

                  return zoneSummaries[left.index].selected && !zoneSummaries[right.index].selected;
              });

    return requestSelectionByIndex(hits.front().index);
}

bool ZoneMapCanvas::requestAuditionAt(juce::Point<float> position)
{
    if (activeGesture.has_value() || !onZoneAuditionRequested)
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
    if (!zone.selected && onZoneSelectionRequested)
        onZoneSelectionRequested(zone.id);
    onZoneAuditionRequested(zone.id, zone.rootKey,
                            std::clamp((zone.velocityLow + zone.velocityHigh) / 2, 1, 127));
    return true;
}

bool ZoneMapCanvas::requestDeleteSelectedSample()
{
    if (!findSelectedZoneIndex().has_value() || !onDeleteSelectedSampleRequested)
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
    return requestSelectionByIndex(static_cast<std::size_t>(nextIndex));
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
        g.setColour(zone.selected ? zoneMapSelectedFill : zoneMapAccentFill);
        g.fillRoundedRectangle(zoneBounds, 8.0f);

        g.setColour(zone.selected ? juce::Colours::white : zoneMapOutline);
        g.drawRoundedRectangle(zoneBounds.reduced(0.75f), 8.0f, zone.selected ? 2.0f : 1.0f);

        auto labelBounds = zoneBounds.toNearestInt().reduced(6, 4);
        labelBounds.setWidth(std::min(labelBounds.getWidth(), 132));
        labelBounds.setHeight(std::min(labelBounds.getHeight(), 20));
        g.setColour(zone.selected ? zoneMapLabelFill.brighter(0.12f) : zoneMapLabelFill);
        g.fillRoundedRectangle(labelBounds.toFloat(), 5.0f);

        g.setColour(juce::Colours::white.withAlpha(zone.selected ? 1.0f : 0.92f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawFittedText(juce::String::fromUTF8(zone.displayName.c_str()),
                         labelBounds.reduced(6, 2),
                         juce::Justification::centredLeft,
                         1);

        if (zone.selected)
        {
            const auto handleCenters = buildHandleCenters(zoneBounds);
            for (const auto& [handle, center] : handleCenters)
            {
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
}

void ZoneMapCanvas::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    if (event.mods.isPopupMenu())
    {
        cancelActiveRangeGesture();
        requestSelectionAt(event.position);
        showContextMenuAt(event.getScreenPosition());
        return;
    }

    if (!beginRangeGestureAt(event.position))
        requestSelectionAt(event.position);
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
    juce::PopupMenu menu;
    menu.addItem(deleteSelectedSampleMenuItemId,
                 "Delete Selected Sample",
                 findSelectedZoneIndex().has_value() && onDeleteSelectedSampleRequested != nullptr);

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
    updateActiveRangeGesture(event.position);
}

void ZoneMapCanvas::mouseUp(const juce::MouseEvent& event)
{
    endActiveRangeGesture(event.position);
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

drs::engine::AuthoringZoneSummary ZoneMapCanvas::getDisplayZoneSummary(std::size_t index) const
{
    if (activeGesture.has_value() && activeGesture->zoneIndex == index)
        return activeGesture->previewZone;

    return zoneSummaries[index];
}

juce::Rectangle<float> ZoneMapCanvas::computeZoneBounds(const drs::engine::AuthoringZoneSummary& zone) const
{
    const auto inner = getInnerBounds();
    const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
    const auto width = std::max(10.0f,
                                inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
    const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
    const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));
    return {x, y, width, height};
}

std::vector<ZoneMapCanvas::ZoneLayout> ZoneMapCanvas::buildZoneLayouts() const
{
    std::vector<ZoneLayout> layouts;
    layouts.reserve(zoneSummaries.size());

    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
        layouts.push_back({index, computeZoneBounds(getDisplayZoneSummary(index))});

    return layouts;
}

std::vector<std::size_t> ZoneMapCanvas::buildPaintOrder() const
{
    std::vector<std::size_t> order;
    order.reserve(zoneSummaries.size());

    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        if (!zoneSummaries[index].selected)
            order.push_back(index);
    }

    if (const auto selectedIndex = findSelectedZoneIndex(); selectedIndex.has_value())
        order.push_back(*selectedIndex);

    return order;
}

std::optional<std::size_t> ZoneMapCanvas::findSelectedZoneIndex() const
{
    for (std::size_t index = 0; index < zoneSummaries.size(); ++index)
    {
        if (getDisplayZoneSummary(index).selected)
            return index;
    }

    return std::nullopt;
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

    for (const auto& [handle, center] : handleCenters)
    {
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

drs::engine::AuthoringZoneSummary ZoneMapCanvas::buildRangePreview(const RangeGesture& gesture,
                                                                   juce::Point<float> position) const
{
    auto preview = gesture.previewZone;

    switch (gesture.handle)
    {
        case RangeHandle::keyLow:
            preview.keyLow = juce::jlimit(0, preview.keyHigh, positionToMidiKey(position));
            break;
        case RangeHandle::keyHigh:
            preview.keyHigh = juce::jlimit(preview.keyLow, 127, positionToMidiKey(position));
            break;
        case RangeHandle::velocityHigh:
            preview.velocityHigh = juce::jlimit(preview.velocityLow, 127, positionToMidiVelocity(position));
            break;
        case RangeHandle::velocityLow:
            preview.velocityLow = juce::jlimit(1, preview.velocityHigh, positionToMidiVelocity(position));
            break;
        case RangeHandle::none:
            break;
    }

    return preview;
}

int ZoneMapCanvas::positionToMidiKey(juce::Point<float> position) const
{
    const auto inner = getInnerBounds();
    const auto proportion = juce::jlimit(0.0f, 1.0f, (position.x - inner.getX()) / inner.getWidth());
    return juce::jlimit(0, 127, static_cast<int>(std::lround(proportion * 127.0f)));
}

int ZoneMapCanvas::positionToMidiVelocity(juce::Point<float> position) const
{
    const auto inner = getInnerBounds();
    const auto proportion = juce::jlimit(0.0f, 1.0f, (position.y - inner.getY()) / inner.getHeight());
    return juce::jlimit(1, 127, static_cast<int>(std::lround((1.0f - proportion) * 127.0f)));
}

bool ZoneMapCanvas::requestSelectionByIndex(std::size_t index)
{
    if (activeGesture.has_value())
        return false;

    if (index >= zoneSummaries.size())
        return false;

    if (zoneSummaries[index].selected)
        return false;

    if (onZoneSelectionRequested)
        onZoneSelectionRequested(zoneSummaries[index].id);

    return onZoneSelectionRequested != nullptr;
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
    gesture.originalZone = getDisplayZoneSummary(zoneIndex);
    gesture.previewZone = gesture.originalZone;
    activeGesture = gesture;
    repaint();
    return true;
}

bool ZoneMapCanvas::updateActiveRangeGesture(juce::Point<float> position)
{
    if (!activeGesture.has_value())
        return false;

    activeGesture->previewZone = buildRangePreview(*activeGesture, position);
    repaint();
    return true;
}

bool ZoneMapCanvas::endActiveRangeGesture(juce::Point<float> position)
{
    if (!activeGesture.has_value())
        return false;

    activeGesture->previewZone = buildRangePreview(*activeGesture, position);
    const auto changed = activeGesture->previewZone.keyLow != activeGesture->originalZone.keyLow
        || activeGesture->previewZone.keyHigh != activeGesture->originalZone.keyHigh
        || activeGesture->previewZone.velocityLow != activeGesture->originalZone.velocityLow
        || activeGesture->previewZone.velocityHigh != activeGesture->originalZone.velocityHigh;

    auto committedZone = activeGesture->previewZone;
    const auto label = (activeGesture->handle == RangeHandle::keyLow || activeGesture->handle == RangeHandle::keyHigh)
        ? std::string("Update zone key range")
        : std::string("Update zone velocity range");
    activeGesture.reset();
    repaint();

    if (changed && onZoneRangeCommitRequested)
        onZoneRangeCommitRequested(committedZone, label);

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
