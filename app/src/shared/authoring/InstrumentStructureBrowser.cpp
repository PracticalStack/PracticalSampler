#include "shared/authoring/InstrumentStructureBrowser.h"
#include "shared/authoring/StructureOverlapPolicy.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace drs::app::authoring
{
namespace
{
bool isDisclosed(const std::vector<std::string>& ids, const std::string& id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool isOpen(const std::vector<std::string>& ids, const std::string& id)
{
    return ids.empty() || isDisclosed(ids, id);
}

bool isOpen(const std::vector<std::string>& disclosedIds,
            const std::vector<std::string>& collapsedIds,
            const std::string& id)
{
    return !isDisclosed(collapsedIds, id)
        && (disclosedIds.empty() || isDisclosed(disclosedIds, id));
}

bool isSelected(const AuthoringStructureSelection& selection, StructureSelectionKind kind, const std::string& id)
{
    const auto expected = kind == StructureSelectionKind::instrument ? StructureSelectionKind::instrument : kind;
    return selection.getKind() == expected && selection.contains(id);
}

std::string display(const std::string& name, const std::string& id)
{
    return name.empty() ? id : name;
}

bool matchesText(const InstrumentStructureRow& row, const std::string& query)
{
    if (query.empty()) return true;
    auto lower = [](std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    };
    const auto needle = lower(query);
    return lower(row.title).find(needle) != std::string::npos
        || lower(row.summary).find(needle) != std::string::npos
        || lower(row.id).find(needle) != std::string::npos;
}
} // namespace

bool InstrumentStructureBrowser::BrowserListBox::keyPressed(const juce::KeyPress& key)
{
    return owner.handleKeyPress(key) || juce::ListBox::keyPressed(key);
}

void InstrumentStructureBrowser::BrowserListBox::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        owner.showContextMenu(getRowContainingPosition(event.getPosition().x, event.getPosition().y));
        return;
    }
    juce::ListBox::mouseDown(event);
}

void InstrumentStructureBrowser::BrowserListBox::mouseDoubleClick(const juce::MouseEvent& event)
{
    owner.toggleDisclosure(getRowContainingPosition(event.getPosition().x, event.getPosition().y));
}

std::vector<InstrumentStructureRow> buildInstrumentStructureRows(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection,
    const std::vector<std::string>& disclosedIds,
    const std::vector<std::string>& collapsedIds,
    const InstrumentStructureBrowserOptions& options)
{
    std::vector<InstrumentStructureRow> result;
    const bool rootOpen = isOpen(disclosedIds, collapsedIds, kInstrumentStructureId);
    result.push_back({ InstrumentStructureRowKind::instrument, kInstrumentStructureId, {},
                       display(project.displayName, "Instrument"),
                       std::to_string(project.authoring.layers.size()) + " layers · "
                           + std::to_string(project.authoring.zones.size()) + " zones",
                       0, rootOpen, isSelected(selection, StructureSelectionKind::instrument, kInstrumentStructureId),
                       selection.getPrimaryId() == kInstrumentStructureId, true,
                       static_cast<int>(project.authoring.layers.size()) });
    if (!rootOpen)
        return result;

    for (const auto& layer : project.authoring.layers)
    {
        int groupCount = 0;
        int zoneCount = 0;
        for (const auto& group : project.authoring.groups)
            if (group.layerId == layer.id)
            {
                ++groupCount;
                zoneCount += static_cast<int>(std::count_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                                             [&](const auto& zone) { return zone.groupId == group.id; }));
            }
        const bool open = isOpen(disclosedIds, collapsedIds, layer.id);
        result.push_back({ InstrumentStructureRowKind::layer, layer.id, kInstrumentStructureId,
                           display(layer.displayName, layer.id),
                           std::to_string(groupCount) + " groups · " + std::to_string(zoneCount) + " zones",
                           1, open,
                           isSelected(selection, StructureSelectionKind::layer, layer.id),
                           selection.getPrimaryId() == layer.id, layer.workspaceVisible, groupCount });
        if (!open)
            continue;
        for (const auto& group : project.authoring.groups)
        {
            if (group.layerId != layer.id)
                continue;
            const int zones = static_cast<int>(std::count_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                                              [&](const auto& zone) { return zone.groupId == group.id; }));
            const bool groupOpen = isOpen(disclosedIds, collapsedIds, group.id);
            result.push_back({ InstrumentStructureRowKind::group, group.id, layer.id,
                               display(group.displayName, group.id),
                               std::to_string(zones) + " zones", 2, groupOpen,
                               isSelected(selection, StructureSelectionKind::group, group.id),
                               selection.getPrimaryId() == group.id, group.workspaceVisible, zones });
            if (!groupOpen)
                continue;
            for (const auto& zone : project.authoring.zones)
            {
                if (zone.groupId != group.id)
                    continue;
                result.push_back({ InstrumentStructureRowKind::zone, zone.id, group.id,
                                   display(zone.displayName, zone.id),
                                   "key " + std::to_string(zone.keyLow) + "–" + std::to_string(zone.keyHigh)
                                       + " · vel " + std::to_string(zone.velocityLow) + "–" + std::to_string(zone.velocityHigh),
                                   3, false,
                                   isSelected(selection, StructureSelectionKind::zone, zone.id),
                                   selection.getPrimaryId() == zone.id,
                                   layer.workspaceVisible && group.workspaceVisible, 0 });
            }
        }
    }
    if (options.searchText.empty() && !options.visibleOnly
        && !options.showOverlapsOnly && !options.showPotentialCollisionsOnly && !options.showExactStacksOnly
        && options.articulationFilter.empty() && !options.performanceEventFilter.has_value())
        return result;

    std::unordered_map<std::string, std::string> parentById;
    std::vector<StructureOverlapInfo> overlapInfo;
    if (options.showOverlapsOnly || options.showPotentialCollisionsOnly || options.showExactStacksOnly)
        overlapInfo = analyzeStructureOverlaps(project.authoring.zones);
    for (const auto& row : result)
        parentById[row.id] = row.parentId;
    std::unordered_set<std::string> keep;
    for (const auto& row : result)
    {
        bool rowMatches = matchesText(row, options.searchText);
        if (row.kind == InstrumentStructureRowKind::zone)
        {
            const auto zone = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                           [&](const auto& candidate) { return candidate.id == row.id; });
            rowMatches = rowMatches && zone != project.authoring.zones.end()
                && (options.articulationFilter.empty() || zone->articulationId == options.articulationFilter)
                && (!options.performanceEventFilter.has_value()
                    || zone->performance.event == *options.performanceEventFilter);
            if (rowMatches && !overlapInfo.empty() && zone != project.authoring.zones.end())
            {
                const auto index = static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), zone));
                if (index >= overlapInfo.size()) rowMatches = false;
                else if (options.showExactStacksOnly)
                    rowMatches = overlapInfo[index].kind == StructureOverlapKind::exactStack
                        || overlapInfo[index].kind == StructureOverlapKind::exactKeyStack;
                else if (options.showPotentialCollisionsOnly)
                    rowMatches = overlapInfo[index].kind == StructureOverlapKind::potentialCollision
                        || overlapInfo[index].hasPotentialCollision;
                else if (options.showOverlapsOnly)
                    rowMatches = overlapInfo[index].kind != StructureOverlapKind::none;
            }
        }
        if (options.visibleOnly && !row.workspaceVisible)
            rowMatches = false;
        if (rowMatches)
        {
            auto id = row.id;
            while (!id.empty())
            {
                keep.insert(id);
                const auto parent = parentById.find(id);
                if (parent == parentById.end() || parent->second.empty()) break;
                id = parent->second;
            }
        }
    }
    result.erase(std::remove_if(result.begin(), result.end(), [&](const auto& row)
                                {
                                    return keep.find(row.id) == keep.end();
                                }),
                 result.end());
    return result;
}

InstrumentStructureBrowser::InstrumentStructureBrowser()
    : list(*this)
{
    title.setText("Instrument Structure", juce::dontSendNotification);
    title.setFont(juce::Font(15.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);
    showZonesButton.setButtonText("Show Zones");
    showZonesButton.setComponentID("authoringShowZonesButton");
    showZonesButton.onClick = [this] { if (onShowZonesRequested) onShowZonesRequested(); };
    addAndMakeVisible(showZonesButton);
    auto configureAction = [this](juce::TextButton& button, const juce::String& text,
                                  const char* id, ActionCallback* callback)
    {
        button.setButtonText(text);
        button.setComponentID(id);
        button.onClick = [this, callback]
        {
            if (*callback) (*callback)();
        };
        addAndMakeVisible(button);
    };
    configureAction(newLayerButton, "+ Layer", "authoringStructureNewLayerButton", &onNewLayerRequested);
    configureAction(newGroupButton, "+ Group", "authoringStructureNewGroupButton", &onNewGroupRequested);
    configureAction(deleteButton, "Delete", "authoringStructureDeleteButton", &onDeleteRequested);
    moreButton.setButtonText("More");
    moreButton.setComponentID("authoringStructureMoreButton");
    addAndMakeVisible(moreButton);
    moreButton.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Select children", static_cast<bool>(onSelectChildrenRequested), false);
        menu.addItem(2, "Select visible children", static_cast<bool>(onSelectVisibleChildrenRequested), false);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&moreButton),
                           [this](const int result)
                           {
                               if (result == 1 && onSelectChildrenRequested) onSelectChildrenRequested();
                               if (result == 2 && onSelectVisibleChildrenRequested) onSelectVisibleChildrenRequested();
                           });
    };
    list.setModel(this);
    list.setRowHeight(30);
    list.setMultipleSelectionEnabled(true);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e272d));
    list.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff39434b));
    list.setComponentID("authoringInstrumentStructureBrowser");
    addAndMakeVisible(list);
}

void InstrumentStructureBrowser::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(54);
    auto titleRow = header.removeFromTop(28);
    const auto showZonesWidth = std::min(112, std::max(82, titleRow.getWidth() / 3));
    showZonesButton.setBounds(titleRow.removeFromRight(showZonesWidth));
    title.setBounds(titleRow);
    auto actionRow = header.removeFromTop(24);
    const auto buttonWidth = std::max(52, (actionRow.getWidth() - 12) / 4);
    newLayerButton.setBounds(actionRow.removeFromLeft(buttonWidth));
    actionRow.removeFromLeft(4);
    newGroupButton.setBounds(actionRow.removeFromLeft(buttonWidth));
    actionRow.removeFromLeft(4);
    deleteButton.setBounds(actionRow.removeFromLeft(buttonWidth));
    actionRow.removeFromLeft(4);
    moreButton.setBounds(actionRow);
    list.setBounds(area);
}

void InstrumentStructureBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12171b));
    g.setColour(juce::Colour(0xff39434b));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.0f);
}

void InstrumentStructureBrowser::setRows(std::vector<InstrumentStructureRow> nextRows)
{
    const auto previousViewPosition = list.getViewport() != nullptr
        ? list.getViewport()->getViewPosition()
        : juce::Point<int> {};
    rows = std::move(nextRows);
    list.updateContent();
    if (list.getViewport() != nullptr)
        list.getViewport()->setViewPosition(previousViewPosition);
    list.repaint();
}

void InstrumentStructureBrowser::setSelection(const AuthoringStructureSelection& nextSelection)
{
    selection = nextSelection;
    suppressCallbacks = true;
    list.deselectAllRows();
    for (int index = 0; index < static_cast<int>(rows.size()); ++index)
        if (rows[static_cast<std::size_t>(index)].selected)
            list.selectRow(index, true, false);
    suppressCallbacks = false;
    list.repaint();
}

void InstrumentStructureBrowser::paintListBoxItem(const int rowNumber, juce::Graphics& g,
                                                  const int width, const int height, const bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(rows.size())) return;
    const auto& row = rows[static_cast<std::size_t>(rowNumber)];
    g.fillAll(rowIsSelected || row.primary ? juce::Colour(0xff304e63) : juce::Colour(0xff1e272d));
    g.setColour(row.kind == InstrumentStructureRowKind::layer ? juce::Colour(0xffe9b35e)
                 : row.kind == InstrumentStructureRowKind::group ? juce::Colour(0xff70c681)
                 : row.kind == InstrumentStructureRowKind::zone ? juce::Colour(0xffa99bdd)
                 : juce::Colours::white);
    g.fillEllipse(static_cast<float>(10 + row.depth * 18), static_cast<float>(height / 2 - 4), 8.0f, 8.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f, row.kind == InstrumentStructureRowKind::instrument ? juce::Font::bold : juce::Font::plain));
    g.drawText(juce::String(row.title), 28 + row.depth * 18, 2, width - 190 - row.depth * 18, height - 4,
               juce::Justification::centredLeft, true);
    g.setColour(juce::Colour(0xffaab5bd));
    g.setFont(juce::Font(11.0f));
    g.drawText(juce::String(row.summary), width - 180, 2, 170, height - 4, juce::Justification::centredRight, true);
}

void InstrumentStructureBrowser::selectedRowsChanged(const int lastRowSelected)
{
    if (suppressCallbacks || lastRowSelected < 0 || lastRowSelected >= static_cast<int>(rows.size())) return;
    handleRowSelection(lastRowSelected, list.isRowSelected(lastRowSelected) && list.getNumSelectedRows() > 1);
}

juce::String InstrumentStructureBrowser::getNameForRow(const int rowNumber)
{
    return rowNumber >= 0 && rowNumber < static_cast<int>(rows.size()) ? rows[static_cast<std::size_t>(rowNumber)].title : juce::String {};
}

juce::String InstrumentStructureBrowser::getTooltipForRow(const int rowNumber)
{
    return rowNumber >= 0 && rowNumber < static_cast<int>(rows.size())
        ? rows[static_cast<std::size_t>(rowNumber)].title + " · " + rows[static_cast<std::size_t>(rowNumber)].summary
        : juce::String {};
}

void InstrumentStructureBrowser::showContextMenu(const int rowNumber)
{
    if (rowNumber >= 0 && rowNumber < static_cast<int>(rows.size()))
    {
        list.selectRow(rowNumber, false, true);
        juce::PopupMenu menu;
        menu.addItem(1, "Show Zones", static_cast<bool>(onShowZonesRequested));
        menu.addItem(2, "Select children", static_cast<bool>(onSelectChildrenRequested));
        menu.addItem(3, "Select visible children", static_cast<bool>(onSelectVisibleChildrenRequested));
        menu.addSeparator();
        menu.addItem(4, "Delete", static_cast<bool>(onDeleteRequested));
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&list),
                           [this](const int result)
                           {
                               if (result == 1 && onShowZonesRequested) onShowZonesRequested();
                               if (result == 2 && onSelectChildrenRequested) onSelectChildrenRequested();
                               if (result == 3 && onSelectVisibleChildrenRequested) onSelectVisibleChildrenRequested();
                               if (result == 4 && onDeleteRequested) onDeleteRequested();
                           });
    }
}

void InstrumentStructureBrowser::toggleDisclosure(const int rowNumber)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(rows.size())) return;
    auto& row = rows[static_cast<std::size_t>(rowNumber)];
    if (row.kind == InstrumentStructureRowKind::zone || row.childCount == 0) return;
    row.disclosed = !row.disclosed;
    if (onDisclosureChanged) onDisclosureChanged(row.id, row.disclosed);
}

void InstrumentStructureBrowser::handleRowSelection(const int rowNumber, const bool additive)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(rows.size())) return;
    // Selection callbacks can synchronously refresh the browser and replace
    // `rows`. Keep the selected row by value so the follow-up scope callback
    // never reads a dangling string/reference after that refresh.
    const auto row = rows[static_cast<std::size_t>(rowNumber)];
    const auto kind = row.kind == InstrumentStructureRowKind::instrument ? StructureSelectionKind::instrument
                     : row.kind == InstrumentStructureRowKind::layer ? StructureSelectionKind::layer
                     : row.kind == InstrumentStructureRowKind::group ? StructureSelectionKind::group
                     : StructureSelectionKind::zone;
    std::vector<std::string> ids { row.id };
    if (additive)
        for (int index = 0; index < static_cast<int>(rows.size()); ++index)
            if (list.isRowSelected(index) && rows[static_cast<std::size_t>(index)].kind == row.kind
                && rows[static_cast<std::size_t>(index)].id != row.id)
                ids.push_back(rows[static_cast<std::size_t>(index)].id);
    if (onSelectionChanged) onSelectionChanged(kind, std::move(ids), row.id);
    if (onScopeRequested && (kind == StructureSelectionKind::layer || kind == StructureSelectionKind::group))
        onScopeRequested(kind == StructureSelectionKind::layer
                             ? StructureScope { StructureScopeKind::layer, row.id }
                             : StructureScope { StructureScopeKind::group, row.id });
}

bool InstrumentStructureBrowser::handleKeyPress(const juce::KeyPress& key)
{
    const auto rowNumber = list.getSelectedRow(0);
    if (rowNumber < 0 || rowNumber >= static_cast<int>(rows.size()))
        return false;

    if (key == juce::KeyPress::returnKey)
    {
        handleRowSelection(rowNumber, false);
        return true;
    }

    const auto& row = rows[static_cast<std::size_t>(rowNumber)];
    if (key == juce::KeyPress::leftKey)
    {
        if (row.kind != InstrumentStructureRowKind::zone && row.disclosed && row.childCount > 0)
        {
            toggleDisclosure(rowNumber);
            return true;
        }
        if (!row.parentId.empty())
        {
            const auto parent = std::find_if(rows.begin(), rows.end(),
                                             [&](const auto& candidate) { return candidate.id == row.parentId; });
            if (parent != rows.end())
                list.selectRow(static_cast<int>(std::distance(rows.begin(), parent)), false, true);
            return true;
        }
    }
    if (key == juce::KeyPress::rightKey)
    {
        if (row.kind != InstrumentStructureRowKind::zone && row.childCount > 0)
        {
            if (!row.disclosed)
            {
                toggleDisclosure(rowNumber);
                return true;
            }
            if (rowNumber + 1 < static_cast<int>(rows.size())
                && rows[static_cast<std::size_t>(rowNumber + 1)].parentId == row.id)
            {
                list.selectRow(rowNumber + 1, false, true);
                return true;
            }
        }
    }
    return false;
}
} // namespace drs::app::authoring
