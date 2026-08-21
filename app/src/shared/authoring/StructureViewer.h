#pragma once

#include "shared/authoring/AuthoringStructureSelection.h"
#include "shared/authoring/StructureViewModels.h"
#include "shared/authoring/StructureZoneList.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <array>
#include <string>
#include <vector>

namespace drs::app::authoring
{
class StructureViewer final : public juce::Component
{
public:
    using SelectionCallback = std::function<void(StructureSelectionKind,
                                                 std::vector<std::string>,
                                                 std::string)>;
    using RevealCallback = std::function<void(const std::string&)>;
    using WidthsCallback = std::function<void(int, int, int)>;

    StructureViewer();

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void setViewModel(StructureHierarchyViewModel nextViewModel);
    void setSelection(const AuthoringStructureSelection& nextSelection);
    void setOnSelectionChanged(SelectionCallback nextCallback);
    void setOnRevealInMapRequested(RevealCallback nextCallback);
    void setOnColumnWidthsChanged(WidthsCallback nextCallback) { onWidthsChanged = std::move(nextCallback); }
    void setColumnWidths(int layerWidth, int groupWidth, int zoneWidth);
    bool revealZone(const std::string& zoneId);
    std::array<int, 3> getColumnWidths() const noexcept { return { layerWidth, groupWidth, zoneWidth }; }

    const StructureHierarchyViewModel& getViewModel() const noexcept { return viewModel; }
    juce::ListBox& getLayerList() noexcept { return layerList; }
    juce::ListBox& getGroupList() noexcept { return groupList; }
    StructureZoneList& getZoneList() noexcept { return zoneList; }

private:
    enum class Column
    {
        layer,
        group,
        zone
    };

    class ColumnModel final : public juce::ListBoxModel
    {
    public:
        ColumnModel(StructureViewer& owner, Column column) : owner(owner), column(column) {}

        int getNumRows() override;
        void paintListBoxItem(int rowNumber,
                              juce::Graphics& g,
                              int width,
                              int height,
                              bool rowIsSelected) override;
        void selectedRowsChanged(int lastRowSelected) override;
        juce::String getNameForRow(int rowNumber) override;
        juce::String getTooltipForRow(int rowNumber) override;

    private:
        StructureViewer& owner;
        Column column;
    };

    static constexpr int headerHeight = 28;
    static constexpr int rowHeight = 48;

    void configureList(juce::ListBox& list,
                       ColumnModel& model,
                       const juce::String& componentId,
                       const juce::String& title,
                       const juce::String& description);
    juce::ListBox& listFor(Column column) noexcept;
    const juce::ListBox& listFor(Column column) const noexcept;
    int rowCountFor(Column column) const noexcept;
    std::string idForRow(Column column, int rowNumber) const;
    StructureSelectionKind kindFor(Column column) const noexcept;
    void handleSelectionChanged(Column column, int lastRowSelected);
    void synchronizeSelection(const AuthoringStructureSelection& nextSelection);
    void paintRow(Column column,
                  int rowNumber,
                  juce::Graphics& g,
                  int width,
                  int height,
                  bool rowIsSelected) const;
    static juce::String keyRangeText(int low, int high);

    juce::Label layerHeader;
    juce::Label groupHeader;
    juce::Label zoneHeader;
    juce::Label zoneRuler;
    juce::Label breadcrumb;
    juce::ListBox layerList;
    juce::ListBox groupList;
    StructureZoneList zoneList;
    ColumnModel layerModel;
    ColumnModel groupModel;
    ColumnModel zoneModel;
    StructureHierarchyViewModel viewModel;
    AuthoringStructureSelection selection;
    SelectionCallback onSelectionChanged;
    RevealCallback onRevealInMapRequested;
    WidthsCallback onWidthsChanged;
    bool suppressCallbacks = false;
    int layerWidth = 188;
    int groupWidth = 220;
    int zoneWidth = 520;
    int dragColumn = -1;
    int dragStartX = 0;
    int dragStartWidth = 0;
};
} // namespace drs::app::authoring
