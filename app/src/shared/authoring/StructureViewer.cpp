#include "shared/authoring/StructureViewer.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <cmath>

namespace drs::app::authoring
{
namespace
{
const auto surface = visual::surfaceSubtle;
const auto border = visual::border;
const auto text = visual::text;
const auto muted = visual::textMuted;
const auto selectedSurface = visual::selection.withAlpha(0.14f);
const auto selectedBorder = visual::selection;
const auto accent = visual::focus;

juce::String eventText(const drs::engine::PerformanceEventKind event)
{
    switch (event)
    {
        case drs::engine::PerformanceEventKind::noteOff: return "note-off";
        case drs::engine::PerformanceEventKind::release: return "release";
        case drs::engine::PerformanceEventKind::pedalDown: return "pedal-down";
        case drs::engine::PerformanceEventKind::pedalUp: return "pedal-up";
        case drs::engine::PerformanceEventKind::controllerChange: return "controller";
        case drs::engine::PerformanceEventKind::noteOn: break;
    }
    return "note-on";
}
}

StructureViewer::StructureViewer()
    : layerModel(*this, Column::layer),
      groupModel(*this, Column::group),
      zoneModel(*this, Column::zone)
{
    setComponentID("authoringStructureViewer");
    setTitle("Structure viewer");
    setDescription("Hierarchical layer, group, and zone browser with aligned zone ranges.");

    for (auto* header : { &layerHeader, &groupHeader, &zoneHeader })
    {
        header->setFont(juce::FontOptions(13.0f, juce::Font::bold));
        header->setColour(juce::Label::textColourId, text);
        header->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(*header);
    }
    layerHeader.setText("Layers", juce::dontSendNotification);
    groupHeader.setText("Groups", juce::dontSendNotification);
    zoneHeader.setText("Zones", juce::dontSendNotification);

    zoneRuler.setFont(juce::FontOptions(10.5f));
    zoneRuler.setColour(juce::Label::textColourId, muted);
    zoneRuler.setJustificationType(juce::Justification::centredLeft);
    zoneRuler.setText("0       24       48       72       96       120   |   Key / velocity", juce::dontSendNotification);
    zoneRuler.setComponentID("authoringStructureZoneRuler");
    addAndMakeVisible(zoneRuler);
    breadcrumb.setComponentID("authoringStructureBreadcrumb");
    breadcrumb.setFont(juce::FontOptions(10.5f));
    breadcrumb.setColour(juce::Label::textColourId, muted);
    breadcrumb.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(breadcrumb);

    configureList(layerList, layerModel, "authoringStructureLayerList", "Structure layers",
                  "Lists authored layers. Selecting a layer reveals its groups.");
    configureList(groupList, groupModel, "authoringStructureGroupList", "Structure groups",
                  "Lists groups in the primary layer. Selecting a group reveals its zones.");
    configureList(zoneList, zoneModel, "authoringStructureZoneList", "Structure zones",
                  "Lists zones in the primary group with aligned key and velocity ranges.");
}

void StructureViewer::configureList(juce::ListBox& list,
                                    ColumnModel& model,
                                    const juce::String& componentId,
                                    const juce::String& title,
                                    const juce::String& description)
{
    list.setComponentID(componentId);
    list.setTitle(title);
    list.setDescription(description);
    list.setModel(&model);
    list.setMultipleSelectionEnabled(true);
    list.setRowHeight(rowHeight);
    list.setOutlineThickness(1);
    list.setColour(juce::ListBox::backgroundColourId, surface);
    list.setColour(juce::ListBox::outlineColourId, border);
    list.setMouseClickGrabsKeyboardFocus(true);
    addAndMakeVisible(list);
}

void StructureViewer::resized()
{
    auto area = getLocalBounds().reduced(6);
    const auto compact = area.getWidth() < 720;
    breadcrumb.setVisible(compact);
    if (compact)
        breadcrumb.setBounds(area.removeFromTop(20));
    else
        breadcrumb.setBounds(juce::Rectangle<int> {});
    const auto availableWidth = area.getWidth();
    const auto minimumLayer = 150;
    const auto minimumGroup = 180;
    const auto minimumZone = 240;
    const auto totalRequested = layerWidth + groupWidth + zoneWidth + 16;
    if (totalRequested > availableWidth)
    {
        const auto deficit = totalRequested - availableWidth;
        const auto groupReduction = std::min(deficit / 2, std::max(0, groupWidth - minimumGroup));
        groupWidth -= groupReduction;
        layerWidth -= std::min(deficit - groupReduction, std::max(0, layerWidth - minimumLayer));
        const auto remaining = layerWidth + groupWidth + zoneWidth + 16 - availableWidth;
        if (remaining > 0)
            zoneWidth = std::max(minimumZone, zoneWidth - remaining);
    }
    else if (totalRequested < availableWidth)
    {
        zoneWidth += availableWidth - totalRequested;
    }
    auto layerArea = area.removeFromLeft(std::max(minimumLayer, layerWidth));
    area.removeFromLeft(8);
    auto groupArea = area.removeFromLeft(std::max(minimumGroup, groupWidth));
    area.removeFromLeft(8);
    auto zoneArea = area;

    layerHeader.setBounds(layerArea.removeFromTop(headerHeight));
    groupHeader.setBounds(groupArea.removeFromTop(headerHeight));
    zoneHeader.setBounds(zoneArea.removeFromTop(headerHeight));
    zoneRuler.setBounds(zoneArea.removeFromTop(18));
    layerList.setBounds(layerArea);
    groupList.setBounds(groupArea);
    zoneList.setBounds(zoneArea);
}

void StructureViewer::setColumnWidths(const int nextLayerWidth,
                                       const int nextGroupWidth,
                                       const int nextZoneWidth)
{
    layerWidth = std::clamp(nextLayerWidth, 150, 520);
    groupWidth = std::clamp(nextGroupWidth, 180, 720);
    zoneWidth = std::clamp(nextZoneWidth, 240, 1400);
    resized();
    if (onWidthsChanged != nullptr)
        onWidthsChanged(layerWidth, groupWidth, zoneWidth);
}

void StructureViewer::mouseDown(const juce::MouseEvent& event)
{
    const auto x = event.position.x;
    const auto layerRight = 6.0f + static_cast<float>(layerWidth);
    const auto groupRight = layerRight + 8.0f + static_cast<float>(groupWidth);
    if (std::abs(x - layerRight) <= 6.0f)
    {
        dragColumn = 0;
        dragStartX = event.getPosition().x;
        dragStartWidth = layerWidth;
    }
    else if (std::abs(x - groupRight) <= 6.0f)
    {
        dragColumn = 1;
        dragStartX = event.getPosition().x;
        dragStartWidth = groupWidth;
    }
    else
        dragColumn = -1;
}

void StructureViewer::mouseDrag(const juce::MouseEvent& event)
{
    if (dragColumn < 0)
        return;
    const auto delta = event.getPosition().x - dragStartX;
    if (dragColumn == 0)
        setColumnWidths(dragStartWidth + delta, groupWidth, zoneWidth);
    else
        setColumnWidths(layerWidth, dragStartWidth + delta, zoneWidth - delta);
}

void StructureViewer::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (onRevealInMapRequested == nullptr || !zoneList.getBounds().contains(event.getPosition()))
        return;
    const auto row = zoneList.getRowContainingPosition(event.getPosition().x - zoneList.getX(),
                                                       event.getPosition().y - zoneList.getY());
    if (row >= 0)
        onRevealInMapRequested(idForRow(Column::zone, row));
}

bool StructureViewer::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
    {
        auto* current = getCurrentlyFocusedComponent();
        if (current == &zoneList || current == &groupList || current == &layerList)
        {
            auto* destination = key == juce::KeyPress::leftKey
                ? (current == &zoneList ? &groupList : current == &groupList ? &layerList : nullptr)
                : (current == &layerList ? &groupList : current == &groupList ? &zoneList : nullptr);
            if (destination != nullptr)
            {
                destination->grabKeyboardFocus();
                return true;
            }
        }
    }
    return false;
}

void StructureViewer::paint(juce::Graphics& g)
{
    g.setColour(visual::surface);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), visual::panelRadius);
    g.setColour(border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), visual::panelRadius, visual::borderWidth);
}

void StructureViewer::setViewModel(StructureHierarchyViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    layerHeader.setText("Layers  " + juce::String(static_cast<int>(viewModel.layers.size())), juce::dontSendNotification);
    groupHeader.setText("Groups  " + juce::String(static_cast<int>(viewModel.groups.size())), juce::dontSendNotification);
    zoneHeader.setText("Zones  " + juce::String(static_cast<int>(viewModel.zones.size())), juce::dontSendNotification);
    layerList.updateContent();
    groupList.updateContent();
    zoneList.updateContent();
    synchronizeSelection(selection);
}

void StructureViewer::setSelection(const AuthoringStructureSelection& nextSelection)
{
    selection = nextSelection;
    auto path = std::string { "Path: Instrument" };
    if (selection.getKind() == StructureSelectionKind::layer)
        path += " > Layer";
    else if (selection.getKind() == StructureSelectionKind::group)
        path += " > Layer > Group";
    else if (selection.getKind() == StructureSelectionKind::zone)
        path += " > Layer > Group > Zone";
    if (!selection.getPrimaryId().empty())
        path += " > " + selection.getPrimaryId();
    breadcrumb.setText(juce::String::fromUTF8(path.c_str()), juce::dontSendNotification);
    synchronizeSelection(selection);
}

void StructureViewer::setOnSelectionChanged(SelectionCallback nextCallback)
{
    onSelectionChanged = std::move(nextCallback);
}

void StructureViewer::setOnRevealInMapRequested(RevealCallback nextCallback)
{
    onRevealInMapRequested = std::move(nextCallback);
}

bool StructureViewer::revealZone(const std::string& zoneId)
{
    for (int row = 0; row < rowCountFor(Column::zone); ++row)
    {
        if (idForRow(Column::zone, row) != zoneId)
            continue;

        suppressCallbacks = true;
        zoneList.selectRow(row, false, true);
        zoneList.scrollToEnsureRowIsOnscreen(row);
        suppressCallbacks = false;
        zoneList.grabKeyboardFocus();
        zoneList.repaint();
        return true;
    }
    return false;
}

juce::ListBox& StructureViewer::listFor(const Column column) noexcept
{
    if (column == Column::layer) return layerList;
    if (column == Column::group) return groupList;
    return zoneList;
}

const juce::ListBox& StructureViewer::listFor(const Column column) const noexcept
{
    if (column == Column::layer) return layerList;
    if (column == Column::group) return groupList;
    return zoneList;
}

int StructureViewer::rowCountFor(const Column column) const noexcept
{
    if (column == Column::layer) return static_cast<int>(viewModel.layers.size());
    if (column == Column::group) return static_cast<int>(viewModel.groups.size());
    return static_cast<int>(viewModel.zones.size());
}

std::string StructureViewer::idForRow(const Column column, const int rowNumber) const
{
    if (rowNumber < 0 || rowNumber >= rowCountFor(column))
        return {};
    if (column == Column::layer) return viewModel.layers[static_cast<std::size_t>(rowNumber)].id;
    if (column == Column::group) return viewModel.groups[static_cast<std::size_t>(rowNumber)].id;
    return viewModel.zones[static_cast<std::size_t>(rowNumber)].id;
}

StructureSelectionKind StructureViewer::kindFor(const Column column) const noexcept
{
    if (column == Column::layer) return StructureSelectionKind::layer;
    if (column == Column::group) return StructureSelectionKind::group;
    return StructureSelectionKind::zone;
}

void StructureViewer::handleSelectionChanged(const Column column, const int lastRowSelected)
{
    if (suppressCallbacks || onSelectionChanged == nullptr || lastRowSelected < 0)
        return;

    auto& list = listFor(column);
    const auto selectedRows = list.getSelectedRows();
    std::vector<std::string> ids;
    ids.reserve(static_cast<std::size_t>(selectedRows.size()));
    for (int index = 0; index < selectedRows.size(); ++index)
    {
        const auto row = selectedRows[index];
        if (const auto id = idForRow(column, row); !id.empty())
            ids.push_back(id);
    }
    const auto primaryId = idForRow(column, lastRowSelected);
    if (!ids.empty() && !primaryId.empty())
        onSelectionChanged(kindFor(column), std::move(ids), primaryId);
}

void StructureViewer::synchronizeSelection(const AuthoringStructureSelection& nextSelection)
{
    const auto sync = [&](const Column column)
    {
        auto& list = listFor(column);
        const auto kind = kindFor(column);
        suppressCallbacks = true;
        list.deselectAllRows();
        if (nextSelection.getKind() == kind)
        {
            for (int index = 0; index < rowCountFor(column); ++index)
            {
                if (nextSelection.contains(idForRow(column, index)))
                    list.selectRow(index, false, false);
            }
            for (int index = 0; index < rowCountFor(column); ++index)
            {
                if (idForRow(column, index) == nextSelection.getPrimaryId())
                {
                    list.scrollToEnsureRowIsOnscreen(index);
                    break;
                }
            }
        }
        suppressCallbacks = false;
        list.repaint();
    };

    sync(Column::layer);
    sync(Column::group);
    sync(Column::zone);
}

void StructureViewer::paintRow(const Column column,
                               const int rowNumber,
                               juce::Graphics& g,
                               const int width,
                               const int height,
                               const bool rowIsSelected) const
{
    const auto id = idForRow(column, rowNumber);
    const auto isPrimary = selection.getPrimaryId() == id && selection.getKind() == kindFor(column);
    auto bounds = juce::Rectangle<float>(1.0f, 1.0f, static_cast<float>(width - 2), static_cast<float>(height - 2));
    g.setColour(rowIsSelected ? selectedSurface : surface);
    g.fillRoundedRectangle(bounds.reduced(1.0f), visual::controlRadius);
    g.setColour(isPrimary ? selectedBorder : border);
    g.drawRoundedRectangle(bounds.reduced(1.0f), visual::controlRadius, isPrimary ? 1.5f : visual::borderWidth);

    auto area = bounds.toNearestInt().reduced(10, 5);
    if (column == Column::layer)
    {
        const auto& row = viewModel.layers[static_cast<std::size_t>(rowNumber)];
        g.setColour(text);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(row.title.c_str()), area.removeFromTop(19), juce::Justification::centredLeft, true);
        g.setColour(muted);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String::fromUTF8(row.statusText.c_str()), area, juce::Justification::centredLeft, true);
        return;
    }

    if (column == Column::group)
    {
        const auto& row = viewModel.groups[static_cast<std::size_t>(rowNumber)];
        g.setColour(visual::stableGroupTint(row.id));
        g.fillEllipse(static_cast<float>(area.getX()), static_cast<float>(area.getY() + 5), 9.0f, 9.0f);
        area.removeFromLeft(15);
        g.setColour(text);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(row.title.c_str()), area.removeFromTop(19), juce::Justification::centredLeft, true);
        g.setColour(muted);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String::fromUTF8(row.statusText.c_str()), area, juce::Justification::centredLeft, true);
        return;
    }

    const auto& row = viewModel.zones[static_cast<std::size_t>(rowNumber)];
    auto metadataArea = area.removeFromBottom(14);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.0f));
    auto metadata = juce::String::fromUTF8(row.articulationId.c_str()) + " · " + eventText(row.performanceEvent);
    if (row.roundRobinLength > 0)
        metadata += " · RR " + juce::String(row.roundRobinPosition) + "/" + juce::String(row.roundRobinLength);
    if (row.hasVelocityCrossfade)
        metadata += " · XFade";
    if (row.hasPotentialCollision)
        metadata += " · potential collision";
    if (row.overlapKind != StructureOverlapKind::none)
        metadata += " · " + juce::String(row.overlapReason.empty() ? "overlap" : row.overlapReason.c_str());
    g.drawFittedText(metadata, metadataArea, juce::Justification::centredLeft, 1);
    auto titleArea = area.removeFromLeft(std::min(154, std::max(88, area.getWidth() / 3)));
    g.setColour(text);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawFittedText(juce::String::fromUTF8(row.title.c_str()), titleArea, juce::Justification::centredLeft, 2);

    auto rangeArea = area.removeFromLeft(std::max(80, area.getWidth() - 92));
    auto barArea = rangeArea.removeFromTop(16).toFloat();
    g.setColour(visual::surfaceHover);
    g.fillRoundedRectangle(barArea, 4.0f);
    const auto x = barArea.getX() + barArea.getWidth() * row.keyStartNormalized;
    const auto right = barArea.getX() + barArea.getWidth() * row.keyEndNormalized;
    g.setColour(visual::stableGroupTint(row.groupId).withAlpha(0.86f));
    g.fillRoundedRectangle({ x, barArea.getY(), std::max(3.0f, right - x), barArea.getHeight() }, 4.0f);
    g.setColour(accent);
    const auto rootX = barArea.getX() + barArea.getWidth() * row.rootNormalized;
    g.fillRect(rootX - 1.0f, barArea.getY() - 2.0f, 2.0f, barArea.getHeight() + 4.0f);
    g.setColour(muted);
    g.setFont(juce::FontOptions(10.5f));
    g.drawText(keyRangeText(row.keyLow, row.keyHigh), rangeArea, juce::Justification::centredLeft, true);
    g.drawText("V " + keyRangeText(row.velocityLow, row.velocityHigh), area, juce::Justification::centredRight, true);
}

juce::String StructureViewer::keyRangeText(const int low, const int high)
{
    return juce::String(low) + "–" + juce::String(high);
}

int StructureViewer::ColumnModel::getNumRows()
{
    return owner.rowCountFor(column);
}

void StructureViewer::ColumnModel::paintListBoxItem(const int rowNumber,
                                                    juce::Graphics& g,
                                                    const int width,
                                                    const int height,
                                                    const bool rowIsSelected)
{
    owner.paintRow(column, rowNumber, g, width, height, rowIsSelected);
}

void StructureViewer::ColumnModel::selectedRowsChanged(const int lastRowSelected)
{
    owner.handleSelectionChanged(column, lastRowSelected);
}

juce::String StructureViewer::ColumnModel::getNameForRow(const int rowNumber)
{
    return juce::String::fromUTF8(owner.idForRow(column, rowNumber).c_str());
}

juce::String StructureViewer::ColumnModel::getTooltipForRow(const int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= owner.rowCountFor(column))
        return {};
    if (column == Column::layer)
        return juce::String::fromUTF8(owner.viewModel.layers[static_cast<std::size_t>(rowNumber)].accessibilityText.c_str());
    if (column == Column::group)
        return juce::String::fromUTF8(owner.viewModel.groups[static_cast<std::size_t>(rowNumber)].accessibilityText.c_str());
    return juce::String::fromUTF8(owner.viewModel.zones[static_cast<std::size_t>(rowNumber)].accessibilityText.c_str());
}
} // namespace drs::app::authoring
