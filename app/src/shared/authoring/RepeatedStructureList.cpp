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
const auto repeatedStructureFocusRing = juce::Colour::fromRGB(24, 29, 33);
const auto repeatedStructureFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 232);

void configureAccessibleMetadata(juce::Component& component,
                                 const juce::String& title,
                                 const juce::String& description,
                                 const juce::String& helpText = {})
{
    component.setTitle(title);
    component.setDescription(description);

    if (helpText.isNotEmpty())
        component.setHelpText(helpText);
}

void drawFocusRing(juce::Graphics& g,
                   juce::Rectangle<float> bounds,
                   float cornerSize,
                   const juce::Colour& outlineColour)
{
    g.setColour(repeatedStructureFocusHalo);
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 3.0f);
    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerSize, 1.8f);
}
} // namespace

class RepeatedStructureList::RowComponent final : public juce::Component
{
public:
    RowComponent()
    {
        setInterceptsMouseClicks(false, true);

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
        titleLabel.setTooltip(title);

        const auto status = juce::String::fromUTF8(row.statusText.c_str());
        statusLabel.setText(status, juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId,
                              row.enabled ? repeatedStructureMuted
                                          : repeatedStructureDisabled);
        statusLabel.setTooltip(status);
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

void RepeatedStructureList::KeyboardNavigableListBox::paint(juce::Graphics& g)
{
    juce::ListBox::paint(g);

    if (hasKeyboardFocus(true))
    {
        drawFocusRing(g,
                      getLocalBounds().toFloat().reduced(2.0f),
                      9.0f,
                      findColour(juce::TextEditor::focusedOutlineColourId));
    }
}

void RepeatedStructureList::KeyboardNavigableListBox::focusGained(FocusChangeType cause)
{
    juce::ListBox::focusGained(cause);
    repaint();
}

void RepeatedStructureList::KeyboardNavigableListBox::focusLost(FocusChangeType cause)
{
    juce::ListBox::focusLost(cause);
    repaint();
}

RepeatedStructureList::RepeatedStructureList(const juce::String& componentId,
                                             const juce::String& listBoxComponentId,
                                             const juce::String& emptyStateComponentId)
    : listBox(*this),
      emptyStateMessage(emptyStateComponentId, juce::Justification::centred)
{
    setComponentID(componentId);
    configureAccessibleMetadata(*this,
                                "Repeated structure list",
                                "Shows a compact selectable list of repeated project structures.");

    listBox.setComponentID(listBoxComponentId);
    listBox.setModel(this);
    listBox.setMultipleSelectionEnabled(false);
    listBox.setRowHeight(24);
    listBox.setOutlineThickness(0);
    listBox.setColour(juce::ListBox::backgroundColourId, repeatedStructureSurface);
    listBox.setColour(juce::ListBox::outlineColourId, repeatedStructureBorder);
    listBox.setColour(juce::TextEditor::focusedOutlineColourId, repeatedStructureFocusRing);
    listBox.setMouseClickGrabsKeyboardFocus(true);
    configureAccessibleMetadata(listBox,
                                "Repeated structure rows",
                                "Lists compact project rows and supports keyboard selection changes.",
                                "Use the up and down arrow keys to move between rows.");

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

void RepeatedStructureList::setSelectedIndex(int nextIndex)
{
    if (viewModel.rows.empty())
        return;

    const auto normalizedIndex = std::clamp(nextIndex, 0, static_cast<int>(viewModel.rows.size()) - 1);
    if (viewModel.selectedIndex == normalizedIndex && listBox.getSelectedRow() == normalizedIndex)
        return;

    suppressSelectionCallback = true;
    viewModel.selectedIndex = normalizedIndex;
    listBox.selectRow(normalizedIndex, false, true);
    listBox.scrollToEnsureRowIsOnscreen(normalizedIndex);
    suppressSelectionCallback = false;
    listBox.repaint();
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
