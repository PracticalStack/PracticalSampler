#include "shared/authoring/MacroWorkbenchView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
constexpr int regionGap = 10;
constexpr int regionInset = 9;
constexpr int headingHeight = 20;
constexpr int rowHeight = 28;
constexpr int compactGap = 5;

void configureHeading(juce::Label& label,
                      const juce::String& text,
                      const juce::String& componentId,
                      const bool sectionHeading)
{
    label.setComponentID(componentId);
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId,
                    sectionHeading ? visual::text : visual::textMuted);
    label.setFont(juce::FontOptions(sectionHeading ? visual::sectionTypeSize
                                                    : visual::compactTypeSize,
                                    sectionHeading ? juce::Font::bold
                                                   : juce::Font::plain));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setTitle(text);
    label.setDescription(sectionHeading ? "Macro workbench region heading."
                                        : "Macro workbench field-group heading.");
}

void drawRegion(juce::Graphics& graphics, const juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return;
    const auto shape = bounds.toFloat().reduced(0.5f);
    graphics.setColour(visual::surface);
    graphics.fillRoundedRectangle(shape, visual::panelRadius);
    graphics.setColour(visual::border);
    graphics.drawRoundedRectangle(shape, visual::panelRadius, visual::borderWidth);
}

void setBoundsIfPresent(juce::Component* component, const juce::Rectangle<int> bounds)
{
    if (component != nullptr)
        component->setBounds(bounds);
}

void clearBounds(std::initializer_list<juce::Component*> components)
{
    for (auto* component : components)
        setBoundsIfPresent(component, {});
}

juce::Rectangle<int> takeTop(juce::Rectangle<int>& area,
                             const int height,
                             const int gapAfter = compactGap)
{
    auto row = area.removeFromTop(std::min(height, area.getHeight()));
    area.removeFromTop(std::min(gapAfter, area.getHeight()));
    return row;
}

void layoutLabelAndField(juce::Rectangle<int> row,
                         juce::Label* label,
                         juce::Component* field,
                         const int labelWidth)
{
    setBoundsIfPresent(label, row.removeFromLeft(std::min(labelWidth, row.getWidth())));
    row.removeFromLeft(std::min(6, row.getWidth()));
    setBoundsIfPresent(field, row);
}
} // namespace

MacroWorkbenchView::MacroWorkbenchView()
{
    setComponentID("authoringMacroContent");
    setTitle("Macro definition and assignment editor");
    setDescription("Edits ordered project macros, identity, range, host exposure, and assigned targets.");

    configureHeading(listHeading, "Macros", "authoringMacroListHeading", true);
    configureHeading(definitionHeading, "Definition", "authoringMacroDefinitionHeading", true);
    configureHeading(identityHeading, "Identity & Host", "authoringMacroIdentityHeading", false);
    configureHeading(rangeHeading, "Range", "authoringMacroRangeHeading", false);
    configureHeading(assignmentsHeading, "Assigned Targets", "authoringMacroAssignmentsHeading", true);
    configureHeading(assignmentDetailHeading, "Selected Target", "authoringMacroAssignmentDetailHeading", false);

    rangeStatusLabel.setComponentID("authoringMacroRangeStatusLabel");
    rangeStatusLabel.setColour(juce::Label::textColourId, visual::textMuted);
    rangeStatusLabel.setFont(juce::FontOptions(visual::metadataTypeSize));
    rangeStatusLabel.setJustificationType(juce::Justification::centredLeft);
    rangeStatusLabel.setTitle("Macro range status");

    assignmentDetailLabel.setComponentID("authoringMacroAssignmentDetailLabel");
    assignmentDetailLabel.setColour(juce::Label::textColourId, visual::textMuted);
    assignmentDetailLabel.setFont(juce::FontOptions(visual::metadataTypeSize));
    assignmentDetailLabel.setJustificationType(juce::Justification::centredLeft);
    assignmentDetailLabel.setTitle("Selected macro target detail");

    definitionEmptyLabel.setComponentID("authoringMacroDefinitionEmptyState");
    definitionEmptyLabel.setText("Create or select a macro to edit its identity, host exposure, and value range.",
                                 juce::dontSendNotification);
    definitionEmptyLabel.setColour(juce::Label::textColourId, visual::textMuted);
    definitionEmptyLabel.setFont(juce::FontOptions(visual::bodyTypeSize));
    definitionEmptyLabel.setJustificationType(juce::Justification::centred);
    definitionEmptyLabel.setTitle("No selected macro");
    definitionEmptyLabel.setDescription(definitionEmptyLabel.getText());

    for (auto* component : { static_cast<juce::Component*>(&listHeading),
                             static_cast<juce::Component*>(&definitionHeading),
                             static_cast<juce::Component*>(&identityHeading),
                             static_cast<juce::Component*>(&rangeHeading),
                             static_cast<juce::Component*>(&rangeStatusLabel),
                             static_cast<juce::Component*>(&assignmentsHeading),
                             static_cast<juce::Component*>(&assignmentDetailHeading),
                             static_cast<juce::Component*>(&assignmentDetailLabel),
                             static_cast<juce::Component*>(&definitionEmptyLabel) })
        addAndMakeVisible(component);
}

void MacroWorkbenchView::setBindings(Bindings nextBindings)
{
    bindings = nextBindings;
    addBoundComponents();
    resized();
}

void MacroWorkbenchView::setPresentationState(const bool hasSelectedMacro,
                                              const juce::String& rangeStatus,
                                              const juce::String& assignmentDetail)
{
    hasSelection = hasSelectedMacro;
    rangeStatusLabel.setText(rangeStatus, juce::dontSendNotification);
    rangeStatusLabel.setDescription(rangeStatus);
    rangeStatusLabel.setTooltip(rangeStatus);
    assignmentDetailLabel.setText(assignmentDetail, juce::dontSendNotification);
    assignmentDetailLabel.setDescription(assignmentDetail);
    assignmentDetailLabel.setTooltip(assignmentDetail);
    definitionEmptyLabel.setVisible(!hasSelection);
    resized();
}

void MacroWorkbenchView::addBoundComponents()
{
    for (auto* component : {
             static_cast<juce::Component*>(bindings.macroList),
             static_cast<juce::Component*>(bindings.createButton),
             static_cast<juce::Component*>(bindings.duplicateButton),
             static_cast<juce::Component*>(bindings.deleteButton),
             static_cast<juce::Component*>(bindings.moveUpButton),
             static_cast<juce::Component*>(bindings.moveDownButton),
             static_cast<juce::Component*>(bindings.nameLabel),
             static_cast<juce::Component*>(bindings.nameEditor),
             static_cast<juce::Component*>(bindings.exposeLabel),
             static_cast<juce::Component*>(bindings.exposeToggle),
             static_cast<juce::Component*>(bindings.roleLabel),
             static_cast<juce::Component*>(bindings.roleSelector),
             static_cast<juce::Component*>(bindings.defaultLabel),
             static_cast<juce::Component*>(bindings.defaultSlider),
             static_cast<juce::Component*>(bindings.minimumLabel),
             static_cast<juce::Component*>(bindings.minimumSlider),
             static_cast<juce::Component*>(bindings.maximumLabel),
             static_cast<juce::Component*>(bindings.maximumSlider),
             static_cast<juce::Component*>(bindings.assignmentList),
             static_cast<juce::Component*>(bindings.assignmentLabel),
             static_cast<juce::Component*>(bindings.assignmentSelector),
             static_cast<juce::Component*>(bindings.addAssignmentButton),
             static_cast<juce::Component*>(bindings.removeAssignmentButton),
             static_cast<juce::Component*>(bindings.assignmentSummary) })
    {
        if (component != nullptr)
            addAndMakeVisible(component);
    }
}

int MacroWorkbenchView::preferredContentHeight(const int contentWidth,
                                               const int viewportHeight,
                                               const bool shortHost) noexcept
{
    if (shortHost || contentWidth < 680)
        return std::max(670, viewportHeight);
    return std::max(1, viewportHeight);
}

void MacroWorkbenchView::paint(juce::Graphics& graphics)
{
    graphics.fillAll(visual::surfaceSubtle);
    drawRegion(graphics, layout.listRegion);
    drawRegion(graphics, layout.definitionRegion);
    drawRegion(graphics, layout.assignmentsRegion);

}

void MacroWorkbenchView::resized()
{
    layout = {};
    auto area = getLocalBounds();
    if (area.isEmpty())
        return;

    if (getWidth() >= 940 && getHeight() >= 205)
    {
        layout.mode = LayoutMode::wideThreeRegion;
        const auto availableWidth = area.getWidth() - regionGap * 2;
        const auto listWidth = std::clamp(static_cast<int>(availableWidth * 0.27f), 250, 330);
        const auto definitionWidth = std::clamp(static_cast<int>(availableWidth * 0.35f), 310, 410);
        layout.listRegion = area.removeFromLeft(listWidth);
        area.removeFromLeft(regionGap);
        layout.definitionRegion = area.removeFromLeft(std::min(definitionWidth, area.getWidth()));
        area.removeFromLeft(std::min(regionGap, area.getWidth()));
        layout.assignmentsRegion = area;
    }
    else if (getWidth() >= 680 && getHeight() < 600)
    {
        layout.mode = LayoutMode::normalListDetail;
        const auto listWidth = std::clamp(static_cast<int>(area.getWidth() * 0.32f), 240, 300);
        layout.listRegion = area.removeFromLeft(listWidth);
        area.removeFromLeft(std::min(regionGap, area.getWidth()));
        const auto definitionWidth = std::max(250, (area.getWidth() - regionGap) * 11 / 20);
        layout.definitionRegion = area.removeFromLeft(
            std::min(definitionWidth, area.getWidth()));
        area.removeFromLeft(std::min(regionGap, area.getWidth()));
        layout.assignmentsRegion = area;
    }
    else
    {
        layout.mode = LayoutMode::compactStacked;
        layout.listRegion = area.removeFromTop(std::min(205, area.getHeight()));
        area.removeFromTop(std::min(regionGap, area.getHeight()));
        layout.definitionRegion = area.removeFromTop(std::min(215, area.getHeight()));
        area.removeFromTop(std::min(regionGap, area.getHeight()));
        layout.assignmentsRegion = area;
    }

    layoutListRegion(layout.listRegion);
    layoutDefinitionRegion(layout.definitionRegion);
    layoutAssignmentsRegion(layout.assignmentsRegion);
}

void MacroWorkbenchView::layoutListRegion(const juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(regionInset, 7);
    listHeading.setBounds(takeTop(area, headingHeight, 4));

    auto primaryActions = takeTop(area, rowHeight, 5);
    const auto actionGap = 5;
    auto createArea = primaryActions.removeFromLeft((primaryActions.getWidth() - actionGap) / 2);
    primaryActions.removeFromLeft(std::min(actionGap, primaryActions.getWidth()));
    setBoundsIfPresent(bindings.createButton, createArea);
    setBoundsIfPresent(bindings.duplicateButton, primaryActions);

    auto footer = area.removeFromBottom(std::min(rowHeight, area.getHeight()));
    area.removeFromBottom(std::min(5, area.getHeight()));
    const auto deleteWidth = std::min(76, footer.getWidth() / 3);
    auto deleteArea = footer.removeFromRight(deleteWidth);
    footer.removeFromRight(std::min(8, footer.getWidth()));
    auto upArea = footer.removeFromLeft((footer.getWidth() - 5) / 2);
    footer.removeFromLeft(std::min(5, footer.getWidth()));
    setBoundsIfPresent(bindings.moveUpButton, upArea);
    setBoundsIfPresent(bindings.moveDownButton, footer);
    setBoundsIfPresent(bindings.deleteButton, deleteArea);
    setBoundsIfPresent(bindings.macroList, area);
}

void MacroWorkbenchView::layoutDefinitionRegion(const juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(regionInset, 7);
    definitionHeading.setBounds(takeTop(area, headingHeight, 3));
    if (!hasSelection)
    {
        clearDefinitionBounds();
        definitionEmptyLabel.setBounds(area.reduced(10));
        return;
    }

    definitionEmptyLabel.setBounds({});
    identityHeading.setBounds(takeTop(area, 17, 3));
    layoutLabelAndField(takeTop(area, rowHeight, 4), bindings.nameLabel,
                        bindings.nameEditor, 64);

    auto identityRow = takeTop(area, rowHeight, 5);
    const auto splitGap = 8;
    auto roleArea = identityRow.removeFromLeft((identityRow.getWidth() - splitGap) * 3 / 5);
    identityRow.removeFromLeft(std::min(splitGap, identityRow.getWidth()));
    layoutLabelAndField(roleArea, bindings.roleLabel, bindings.roleSelector, 34);
    layoutLabelAndField(identityRow, bindings.exposeLabel, bindings.exposeToggle, 42);

    rangeHeading.setBounds(takeTop(area, 17, 3));
    auto rangeRow = takeTop(area, 32, 3);
    constexpr int rangeGap = 7;
    const auto columnWidth = std::max(1, (rangeRow.getWidth() - rangeGap * 2) / 3);
    auto defaultArea = rangeRow.removeFromLeft(columnWidth);
    rangeRow.removeFromLeft(std::min(rangeGap, rangeRow.getWidth()));
    auto minimumArea = rangeRow.removeFromLeft(columnWidth);
    rangeRow.removeFromLeft(std::min(rangeGap, rangeRow.getWidth()));
    layoutLabelAndField(defaultArea, bindings.defaultLabel, bindings.defaultSlider, 48);
    layoutLabelAndField(minimumArea, bindings.minimumLabel, bindings.minimumSlider, 30);
    layoutLabelAndField(rangeRow, bindings.maximumLabel, bindings.maximumSlider, 30);
    rangeStatusLabel.setBounds(area.removeFromTop(std::min(24, area.getHeight())));
}

void MacroWorkbenchView::layoutAssignmentsRegion(const juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(regionInset, 7);
    assignmentsHeading.setBounds(takeTop(area, headingHeight, 3));
    if (!hasSelection)
    {
        clearAssignmentBounds();
        assignmentDetailLabel.setText("Create or select a macro before assigning a target.",
                                      juce::dontSendNotification);
        assignmentDetailLabel.setBounds(area.reduced(10));
        return;
    }

    auto detailArea = area.removeFromBottom(std::min(92, std::max(72, area.getHeight() / 2)));
    area.removeFromBottom(std::min(5, area.getHeight()));
    setBoundsIfPresent(bindings.assignmentList, area);

    assignmentDetailHeading.setBounds(takeTop(detailArea, 16, 2));
    layoutLabelAndField(takeTop(detailArea, rowHeight, 3), bindings.assignmentLabel,
                        bindings.assignmentSelector, 44);
    auto actionRow = takeTop(detailArea, rowHeight, 2);
    auto addArea = actionRow.removeFromLeft((actionRow.getWidth() - 6) / 2);
    actionRow.removeFromLeft(std::min(6, actionRow.getWidth()));
    setBoundsIfPresent(bindings.addAssignmentButton, addArea);
    setBoundsIfPresent(bindings.removeAssignmentButton, actionRow);
    auto summaryRow = detailArea;
    const auto detailWidth = std::max(1, summaryRow.getWidth() * 2 / 5);
    assignmentDetailLabel.setBounds(summaryRow.removeFromLeft(detailWidth));
    summaryRow.removeFromLeft(std::min(5, summaryRow.getWidth()));
    setBoundsIfPresent(bindings.assignmentSummary, summaryRow);
}

void MacroWorkbenchView::clearDefinitionBounds()
{
    identityHeading.setBounds({});
    rangeHeading.setBounds({});
    rangeStatusLabel.setBounds({});
    clearBounds({ bindings.nameLabel, bindings.nameEditor,
                  bindings.exposeLabel, bindings.exposeToggle,
                  bindings.roleLabel, bindings.roleSelector,
                  bindings.defaultLabel, bindings.defaultSlider,
                  bindings.minimumLabel, bindings.minimumSlider,
                  bindings.maximumLabel, bindings.maximumSlider });
}

void MacroWorkbenchView::clearAssignmentBounds()
{
    assignmentDetailHeading.setBounds({});
    clearBounds({ bindings.assignmentList, bindings.assignmentLabel,
                  bindings.assignmentSelector, bindings.addAssignmentButton,
                  bindings.removeAssignmentButton, bindings.assignmentSummary });
}
} // namespace drs::app::authoring
