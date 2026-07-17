#pragma once

#include "shared/authoring/AuthoringViewModels.h"
#include "shared/authoring/CompactInspectorPrimitives.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class RepeatedStructureList final : public juce::Component,
                                    private juce::ListBoxModel
{
public:
    RepeatedStructureList(const juce::String& componentId,
                          const juce::String& listBoxComponentId,
                          const juce::String& emptyStateComponentId);

    void resized() override;

    void setViewModel(RepeatedStructureListViewModel nextViewModel);
    const RepeatedStructureListViewModel& getViewModel() const { return viewModel; }

    void setOnSelectionChanged(RepeatedStructureSelectionCallback nextCallback);
    int getSelectedIndex() const { return viewModel.selectedIndex; }
    int getRowCount() const { return static_cast<int>(viewModel.rows.size()); }
    juce::ListBox& getListBox() { return listBox; }
    const juce::ListBox& getListBox() const { return listBox; }

private:
    class KeyboardNavigableListBox final : public juce::ListBox
    {
    public:
        explicit KeyboardNavigableListBox(RepeatedStructureList& owner);
        bool keyPressed(const juce::KeyPress& key) override;
        void paint(juce::Graphics& g) override;
        void focusGained(FocusChangeType cause) override;
        void focusLost(FocusChangeType cause) override;

    private:
        RepeatedStructureList& owner;
    };

    class RowComponent;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber,
                                            bool isRowSelected,
                                            juce::Component* existingComponentToUpdate) override;

    bool moveSelectionBy(int direction);
    void refreshVisibility();

    KeyboardNavigableListBox listBox;
    CompactInspectorMessage emptyStateMessage;
    RepeatedStructureListViewModel viewModel;
    RepeatedStructureSelectionCallback onSelectionChanged;
    bool suppressSelectionCallback = false;
};
} // namespace drs::app::authoring
