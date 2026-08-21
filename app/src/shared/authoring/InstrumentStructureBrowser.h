#pragma once

#include "shared/authoring/AuthoringStructureSelection.h"
#include "shared/authoring/StructureScope.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace drs::app::authoring
{
enum class InstrumentStructureRowKind { instrument, layer, group, zone };

struct InstrumentStructureRow
{
    InstrumentStructureRowKind kind = InstrumentStructureRowKind::instrument;
    std::string id;
    std::string parentId;
    std::string title;
    std::string summary;
    int depth = 0;
    bool disclosed = true;
    bool selected = false;
    bool primary = false;
    bool workspaceVisible = true;
    int childCount = 0;
};

struct InstrumentStructureBrowserOptions
{
    std::string searchText;
    bool visibleOnly = false;
    bool showOverlapsOnly = false;
    bool showPotentialCollisionsOnly = false;
    bool showExactStacksOnly = false;
    std::string articulationFilter;
    std::optional<drs::engine::PerformanceEventKind> performanceEventFilter;
};

class InstrumentStructureBrowser final : public juce::Component,
                                          private juce::ListBoxModel
{
public:
    using SelectionCallback = std::function<void(StructureSelectionKind,
                                                  std::vector<std::string>,
                                                  std::string)>;
    using ScopeCallback = std::function<void(StructureScope)>;
    using ActionCallback = std::function<void()>;
    using DisclosureCallback = std::function<void(std::string, bool)>;

    InstrumentStructureBrowser();
    void resized() override;
    void paint(juce::Graphics& g) override;

    void setRows(std::vector<InstrumentStructureRow> nextRows);
    void setSelection(const AuthoringStructureSelection& nextSelection);
    void setOnSelectionChanged(SelectionCallback callback) { onSelectionChanged = std::move(callback); }
    void setOnScopeRequested(ScopeCallback callback) { onScopeRequested = std::move(callback); }
    void setOnShowZonesRequested(ActionCallback callback) { onShowZonesRequested = std::move(callback); }
    void setOnNewLayerRequested(ActionCallback callback) { onNewLayerRequested = std::move(callback); }
    void setOnNewGroupRequested(ActionCallback callback) { onNewGroupRequested = std::move(callback); }
    void setOnDeleteRequested(ActionCallback callback) { onDeleteRequested = std::move(callback); }
    void setOnSelectChildrenRequested(ActionCallback callback) { onSelectChildrenRequested = std::move(callback); }
    void setOnSelectVisibleChildrenRequested(ActionCallback callback) { onSelectVisibleChildrenRequested = std::move(callback); }
    void setOnDisclosureChanged(DisclosureCallback callback) { onDisclosureChanged = std::move(callback); }

    const std::vector<InstrumentStructureRow>& getRows() const noexcept { return rows; }
    juce::ListBox& getList() noexcept { return list; }
    void setScrollAnchor(int row) noexcept { scrollAnchor = std::max(0, row); }
    int getScrollAnchor() const noexcept { return scrollAnchor; }

private:
    class BrowserListBox final : public juce::ListBox
    {
    public:
        explicit BrowserListBox(InstrumentStructureBrowser& owner) : owner(owner) {}
        bool keyPressed(const juce::KeyPress& key) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;
    private:
        InstrumentStructureBrowser& owner;
    };

    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    juce::String getNameForRow(int rowNumber) override;
    juce::String getTooltipForRow(int rowNumber) override;
    void showContextMenu(int rowNumber);
    void toggleDisclosure(int rowNumber);
    void handleRowSelection(int rowNumber, bool additive);
    bool handleKeyPress(const juce::KeyPress& key);

    juce::Label title;
    juce::TextButton showZonesButton;
    juce::TextButton newLayerButton;
    juce::TextButton newGroupButton;
    juce::TextButton deleteButton;
    juce::TextButton moreButton;
    BrowserListBox list;
    std::vector<InstrumentStructureRow> rows;
    AuthoringStructureSelection selection;
    SelectionCallback onSelectionChanged;
    ScopeCallback onScopeRequested;
    ActionCallback onShowZonesRequested;
    ActionCallback onNewLayerRequested;
    ActionCallback onNewGroupRequested;
    ActionCallback onDeleteRequested;
    ActionCallback onSelectChildrenRequested;
    ActionCallback onSelectVisibleChildrenRequested;
    DisclosureCallback onDisclosureChanged;
    bool suppressCallbacks = false;
    int scrollAnchor = 0;
};

std::vector<InstrumentStructureRow> buildInstrumentStructureRows(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection,
    const std::vector<std::string>& disclosedIds = {},
    const std::vector<std::string>& collapsedIds = {},
    const InstrumentStructureBrowserOptions& options = {});
} // namespace drs::app::authoring
