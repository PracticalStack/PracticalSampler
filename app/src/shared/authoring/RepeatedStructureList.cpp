#include "shared/authoring/RepeatedStructureList.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
const auto repeatedStructureSurface = juce::Colour::fromRGB(244, 239, 231);
const auto repeatedStructureBorder = juce::Colour::fromRGB(214, 202, 187);
const auto repeatedStructureSelected = juce::Colour::fromRGB(28, 108, 88);
const auto repeatedStructureSelectedSurface = repeatedStructureSelected.withAlpha(0.14f);
const auto repeatedStructureTitle = juce::Colour::fromRGB(24, 29, 33);
const auto repeatedStructureMuted = juce::Colour::fromRGB(82, 86, 94);
const auto repeatedStructureDisabled = juce::Colour::fromRGB(151, 154, 160);
} // namespace

class RepeatedStructureList::RowComponent final : public juce::Component
{
public:
    RowComponent()
    {
        titleLabel.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setInterceptsMouseClicks(false, false);

        statusLabel.setFont(juce::FontOptions(11.0f));
        statusLabel.setJustificationType(juce::Justification::centredRight);
        statusLabel.setInterceptsMouseClicks(false, false);

        addAndMakeVisible(titleLabel);
        addAndMakeVisible(statusLabel);
    }

    void setViewModel(const RepeatedStructureRowViewModel& nextRow, bool nextSelected)
    {
        row = nextRow;
        selected = nextSelected;

        const auto title = row.title.empty() ? "(unnamed item)" : juce::String::fromUTF8(row.title.c_str());
        titleLabel.setText(title, juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId,
                             row.enabled ? repeatedStructureTitle
                                         : repeatedStructureDisabled);

        statusLabel.setText(juce::String::fromUTF8(row.statusText.c_str()),
                            juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId,
                              row.enabled ? repeatedStructureMuted
                                          : repeatedStructureDisabled);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(selected ? repeatedStructureSelectedSurface : repeatedStructureSurface);
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(selected ? repeatedStructureSelected : repeatedStructureBorder);
        g.drawRoundedRectangle(bounds, 8.0f, selected ? 1.4f : 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 4);
        auto titleArea = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.56f));
        titleLabel.setBounds(titleArea);
        statusLabel.setBounds(area);
    }

private:
    RepeatedStructureRowViewModel row;
    bool selected = false;
    juce::Label titleLabel;
    juce::Label statusLabel;
};

RepeatedStructureList::KeyboardNavigableListBox::KeyboardNavigableListBox(RepeatedStructureList& nextOwner)
    : owner(nextOwner)
{
}

bool RepeatedStructureList::KeyboardNavigableListBox::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey)
        return owner.moveSelectionBy(-1);

    if (key == juce::KeyPress::downKey)
        return owner.moveSelectionBy(1);

    return juce::ListBox::keyPressed(key);
}

RepeatedStructureList::RepeatedStructureList(const juce::String& componentId,
                                             const juce::String& listBoxComponentId,
                                             const juce::String& emptyStateComponentId)
    : listBox(*this),
      emptyStateMessage(emptyStateComponentId, juce::Justification::centred)
{
    setComponentID(componentId);

    listBox.setComponentID(listBoxComponentId);
    listBox.setModel(this);
    listBox.setMultipleSelectionEnabled(false);
    listBox.setRowHeight(24);
    listBox.setOutlineThickness(0);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);

    addAndMakeVisible(listBox);
    addAndMakeVisible(emptyStateMessage);
    refreshVisibility();
}

void RepeatedStructureList::resized()
{
    const auto bounds = getLocalBounds();
    listBox.setBounds(bounds);
    emptyStateMessage.setBounds(bounds.reduced(6));
}

void RepeatedStructureList::setViewModel(RepeatedStructureListViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    suppressSelectionCallback = true;
    listBox.updateContent();

    if (viewModel.rows.empty())
    {
        listBox.deselectAllRows();
        viewModel.selectedIndex = -1;
    }
    else
    {
        viewModel.selectedIndex = std::clamp(viewModel.selectedIndex, 0, static_cast<int>(viewModel.rows.size()) - 1);
        listBox.selectRow(viewModel.selectedIndex, false, true);
        listBox.scrollToEnsureRowIsOnscreen(viewModel.selectedIndex);
    }

    suppressSelectionCallback = false;
    emptyStateMessage.setText(juce::String::fromUTF8(viewModel.emptyStateText.c_str()));
    refreshVisibility();
    repaint();
}

void RepeatedStructureList::setOnSelectionChanged(RepeatedStructureSelectionCallback nextCallback)
{
    onSelectionChanged = std::move(nextCallback);
}

int RepeatedStructureList::getNumRows()
{
    return static_cast<int>(viewModel.rows.size());
}

void RepeatedStructureList::paintListBoxItem(int, juce::Graphics&, int, int, bool)
{
}

void RepeatedStructureList::selectedRowsChanged(int lastRowSelected)
{
    if (suppressSelectionCallback)
        return;

    viewModel.selectedIndex = lastRowSelected;
    listBox.repaint();

    if (onSelectionChanged != nullptr && lastRowSelected >= 0)
        onSelectionChanged(lastRowSelected);
}

juce::Component* RepeatedStructureList::refreshComponentForRow(int rowNumber,
                                                               bool isRowSelected,
                                                               juce::Component* existingComponentToUpdate)
{
    auto* rowComponent = dynamic_cast<RowComponent*>(existingComponentToUpdate);
    if (rowComponent == nullptr)
        rowComponent = new RowComponent();

    if (rowNumber >= 0 && static_cast<std::size_t>(rowNumber) < viewModel.rows.size())
        rowComponent->setViewModel(viewModel.rows[static_cast<std::size_t>(rowNumber)], isRowSelected);

    return rowComponent;
}

bool RepeatedStructureList::moveSelectionBy(int direction)
{
    if (viewModel.rows.empty())
        return false;

    const auto lastIndex = static_cast<int>(viewModel.rows.size()) - 1;
    const auto currentIndex = std::clamp(listBox.getSelectedRow(), 0, lastIndex);
    const auto nextIndex = std::clamp(currentIndex + direction, 0, lastIndex);

    if (nextIndex == currentIndex)
        return true;

    listBox.selectRow(nextIndex, true, true);
    listBox.scrollToEnsureRowIsOnscreen(nextIndex);
    return true;
}

void RepeatedStructureList::refreshVisibility()
{
    const auto hasRows = !viewModel.rows.empty();
    listBox.setVisible(hasRows);
    emptyStateMessage.setVisible(!hasRows);
}
} // namespace drs::app::authoring
