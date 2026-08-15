#include "shared/authoring/RoutingWorkbenchView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <iterator>

namespace drs::app::authoring
{
namespace
{
constexpr int regionGap = 10;
constexpr int regionInset = 9;
constexpr int rowGap = 4;
constexpr int headingHeight = 20;
constexpr int controlRowHeight = 28;

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
}

void configureMetadata(juce::Label& label,
                       const juce::String& componentId,
                       const juce::String& title)
{
    label.setComponentID(componentId);
    label.setColour(juce::Label::textColourId, visual::textMuted);
    label.setFont(juce::FontOptions(visual::metadataTypeSize));
    label.setJustificationType(juce::Justification::centredLeft);
    label.setTitle(title);
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

void setBoundsIfPresent(juce::Component* component, juce::Rectangle<int> bounds)
{
    if (component != nullptr)
        component->setBounds(bounds);
}

juce::Rectangle<int> takeTop(juce::Rectangle<int>& area,
                             const int height,
                             const int gapAfter = rowGap)
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

void splitFields(juce::Rectangle<int> row,
                 std::initializer_list<std::pair<juce::Component*, int>> fields,
                 const int gap = 6)
{
    auto remainingWeight = 0;
    for (const auto& field : fields)
        remainingWeight += field.second;

    auto iterator = fields.begin();
    while (iterator != fields.end())
    {
        const auto isLast = std::next(iterator) == fields.end();
        const auto width = isLast || remainingWeight <= 0
            ? row.getWidth()
            : row.getWidth() * iterator->second / remainingWeight;
        setBoundsIfPresent(iterator->first, row.removeFromLeft(width));
        remainingWeight -= iterator->second;
        if (!isLast)
            row.removeFromLeft(std::min(gap, row.getWidth()));
        ++iterator;
    }
}
} // namespace

RoutingWorkbenchView::RoutingWorkbenchView()
{
    setComponentID("authoringRoutingContent");
    setTitle("Routing signal path and selected FX editor");
    setDescription("Edits routing buses, insert order, selected FX identity, parameters, and Macro control assignment.");

    configureHeading(signalPathHeading, "Ordered Signal Path",
                     "authoringRoutingSignalPathHeading", false);
    configureMetadata(signalPathLabel, "authoringRoutingSignalPathLabel",
                      "Selected bus signal path");
    configureHeading(selectedFxIdentityHeading, "Identity, Owner & Actions",
                     "authoringFxIdentityHeading", false);
    configureMetadata(selectedFxContextLabel, "authoringFxContextLabel",
                      "Selected FX owner and position");
    configureHeading(parameterHeading, "Parameter & Macro Control",
                     "authoringFxParameterHeading", false);
    configureMetadata(macroControlLabel, "authoringFxMacroAssignmentSummary",
                      "Selected parameter Macro control status");

    configureMetadata(busEmptyLabel, "authoringRoutingEmptyState", "No routing bus");
    busEmptyLabel.setText("No routing buses are authored. Choose a scope and use Add Insert to create its first signal path.",
                          juce::dontSendNotification);
    configureMetadata(fxEmptyLabel, "authoringFxEmptyState", "No selected FX");
    fxEmptyLabel.setText("This signal path has no selected insert. Use Add Insert to create one.",
                         juce::dontSendNotification);

    for (auto* component : { static_cast<juce::Component*>(&signalPathHeading),
                             static_cast<juce::Component*>(&signalPathLabel),
                             static_cast<juce::Component*>(&selectedFxIdentityHeading),
                             static_cast<juce::Component*>(&selectedFxContextLabel),
                             static_cast<juce::Component*>(&parameterHeading),
                             static_cast<juce::Component*>(&macroControlLabel),
                             static_cast<juce::Component*>(&busEmptyLabel),
                             static_cast<juce::Component*>(&fxEmptyLabel) })
        addAndMakeVisible(component);
}

void RoutingWorkbenchView::setBindings(Bindings nextBindings)
{
    bindings = nextBindings;
    addBoundComponents();
    resized();
}

void RoutingWorkbenchView::setPresentationState(const bool hasRoutingBus,
                                                const bool hasSelectedFx,
                                                const bool warningState,
                                                const juce::String& signalPath,
                                                const juce::String& selectedFxContext,
                                                const juce::String& macroControlSummary)
{
    hasBus = hasRoutingBus;
    hasFx = hasSelectedFx;
    hasWarning = warningState;
    signalPathLabel.setText(signalPath, juce::dontSendNotification);
    signalPathLabel.setDescription(signalPath);
    signalPathLabel.setTooltip(signalPath);
    selectedFxContextLabel.setText(selectedFxContext, juce::dontSendNotification);
    selectedFxContextLabel.setDescription(selectedFxContext);
    selectedFxContextLabel.setTooltip(selectedFxContext);
    macroControlLabel.setText(macroControlSummary, juce::dontSendNotification);
    macroControlLabel.setDescription(macroControlSummary);
    macroControlLabel.setTooltip(macroControlSummary);
    busEmptyLabel.setVisible(!hasBus);
    fxEmptyLabel.setVisible(!hasFx);
    if (bindings.fxDiagnosticsLabel != nullptr)
        bindings.fxDiagnosticsLabel->setColour(juce::Label::textColourId,
                                               hasWarning ? visual::warning : visual::textMuted);
    repaint();
    resized();
}

void RoutingWorkbenchView::addBoundComponents()
{
    for (auto* component : {
             static_cast<juce::Component*>(bindings.fxSectionLabel),
             static_cast<juce::Component*>(bindings.scopeLabel),
             static_cast<juce::Component*>(bindings.scopeSelector),
             static_cast<juce::Component*>(bindings.scopeBreadcrumb),
             static_cast<juce::Component*>(bindings.fxSelector),
             static_cast<juce::Component*>(bindings.fxNameEditor),
             static_cast<juce::Component*>(bindings.fxTypeLabel),
             static_cast<juce::Component*>(bindings.fxTypeSelector),
             static_cast<juce::Component*>(bindings.fxBypassedToggle),
             static_cast<juce::Component*>(bindings.fxAddButton),
             static_cast<juce::Component*>(bindings.fxDuplicateButton),
             static_cast<juce::Component*>(bindings.fxMoveUpButton),
             static_cast<juce::Component*>(bindings.fxMoveDownButton),
             static_cast<juce::Component*>(bindings.fxDeleteButton),
             static_cast<juce::Component*>(bindings.fxOwnerSelector),
             static_cast<juce::Component*>(bindings.fxMoveOwnerButton),
             static_cast<juce::Component*>(bindings.fxParameterSelector),
             static_cast<juce::Component*>(bindings.fxParameterSlider),
             static_cast<juce::Component*>(bindings.fxParameterResetButton),
             static_cast<juce::Component*>(bindings.fxAssignMacroButton),
             static_cast<juce::Component*>(bindings.fxParameterValueLabel),
             static_cast<juce::Component*>(bindings.fxSummaryLabel),
             static_cast<juce::Component*>(bindings.fxDiagnosticsLabel),
             static_cast<juce::Component*>(bindings.routingSectionLabel),
             static_cast<juce::Component*>(bindings.routingBusSelector),
             static_cast<juce::Component*>(bindings.routingInputLabel),
             static_cast<juce::Component*>(bindings.routingInputSelector),
             static_cast<juce::Component*>(bindings.routingInsertOneLabel),
             static_cast<juce::Component*>(bindings.routingInsertOneSelector),
             static_cast<juce::Component*>(bindings.routingInsertTwoLabel),
             static_cast<juce::Component*>(bindings.routingInsertTwoSelector),
             static_cast<juce::Component*>(bindings.routingSummaryLabel) })
    {
        if (component != nullptr)
            addAndMakeVisible(component);
    }
}

int RoutingWorkbenchView::preferredContentHeight(const int contentWidth,
                                                 const int viewportHeight,
                                                 const bool shortHost) noexcept
{
    if (shortHost || contentWidth < 680)
        return std::max(620, viewportHeight);
    return std::max(320, viewportHeight + 18);
}

void RoutingWorkbenchView::paint(juce::Graphics& graphics)
{
    graphics.fillAll(visual::surfaceSubtle);
    drawRegion(graphics, layout.signalPathRegion);
    drawRegion(graphics, layout.selectedFxRegion);

    if (hasWarning && !layout.selectedFxRegion.isEmpty())
    {
        graphics.setColour(visual::warning);
        const auto warningLine = layout.selectedFxRegion.reduced(1).removeFromLeft(3).toFloat();
        graphics.fillRect(warningLine);
    }
}

void RoutingWorkbenchView::resized()
{
    layout = {};
    auto area = getLocalBounds();
    if (area.isEmpty())
        return;

    if (getHeight() >= 500 || getWidth() < 680)
    {
        layout.mode = LayoutMode::compactStacked;
        layout.signalPathRegion = area.removeFromTop(std::min(265, area.getHeight()));
        area.removeFromTop(std::min(regionGap, area.getHeight()));
        layout.selectedFxRegion = area;
    }
    else
    {
        layout.mode = getWidth() >= 920 ? LayoutMode::wideTwoRegion
                                        : LayoutMode::normalBalanced;
        const auto signalFraction = layout.mode == LayoutMode::wideTwoRegion ? 37 : 43;
        const auto signalWidth = std::clamp(
            (area.getWidth() - regionGap) * signalFraction / 100, 300, 430);
        layout.signalPathRegion = area.removeFromLeft(signalWidth);
        area.removeFromLeft(std::min(regionGap, area.getWidth()));
        layout.selectedFxRegion = area;
    }

    layoutSignalPathRegion(layout.signalPathRegion);
    layoutSelectedFxRegion(layout.selectedFxRegion);
}

void RoutingWorkbenchView::layoutSignalPathRegion(const juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(regionInset, 7);
    setBoundsIfPresent(bindings.routingSectionLabel, takeTop(area, headingHeight, 3));

    auto scopeRow = takeTop(area, controlRowHeight, 3);
    const auto scopeWidth = std::min(210, std::max(150, scopeRow.getWidth() * 2 / 5));
    auto selectorArea = scopeRow.removeFromLeft(scopeWidth);
    layoutLabelAndField(selectorArea, bindings.scopeLabel, bindings.scopeSelector, 42);
    scopeRow.removeFromLeft(std::min(7, scopeRow.getWidth()));
    setBoundsIfPresent(bindings.scopeBreadcrumb, scopeRow);

    if (!hasBus)
    {
        setBoundsIfPresent(bindings.routingBusSelector, {});
        setBoundsIfPresent(bindings.routingInputLabel, {});
        setBoundsIfPresent(bindings.routingInputSelector, {});
        signalPathHeading.setBounds({});
        setBoundsIfPresent(bindings.routingInsertOneLabel, {});
        setBoundsIfPresent(bindings.routingInsertOneSelector, {});
        setBoundsIfPresent(bindings.routingInsertTwoLabel, {});
        setBoundsIfPresent(bindings.routingInsertTwoSelector, {});
        signalPathLabel.setBounds({});
        setBoundsIfPresent(bindings.routingSummaryLabel, {});
        busEmptyLabel.setBounds(area.reduced(10));
        return;
    }

    busEmptyLabel.setBounds({});
    setBoundsIfPresent(bindings.routingBusSelector, takeTop(area, controlRowHeight));
    layoutLabelAndField(takeTop(area, controlRowHeight), bindings.routingInputLabel,
                        bindings.routingInputSelector, 44);
    signalPathHeading.setBounds(takeTop(area, 17, 3));
    layoutLabelAndField(takeTop(area, controlRowHeight, 3), bindings.routingInsertOneLabel,
                        bindings.routingInsertOneSelector, 52);
    layoutLabelAndField(takeTop(area, controlRowHeight, 3), bindings.routingInsertTwoLabel,
                        bindings.routingInsertTwoSelector, 52);
    signalPathLabel.setBounds(takeTop(area, 18, 2));
    setBoundsIfPresent(bindings.routingSummaryLabel, area.removeFromTop(std::max(1, area.getHeight())));
}

void RoutingWorkbenchView::layoutSelectedFxRegion(const juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(regionInset, 7);
    setBoundsIfPresent(bindings.fxSectionLabel, takeTop(area, headingHeight, 3));
    setBoundsIfPresent(bindings.fxSelector, takeTop(area, controlRowHeight, 3));

    if (!hasFx)
    {
        selectedFxIdentityHeading.setBounds({});
        selectedFxContextLabel.setBounds({});
        parameterHeading.setBounds({});
        macroControlLabel.setBounds({});
        for (auto* component : { static_cast<juce::Component*>(bindings.fxNameEditor),
                                 static_cast<juce::Component*>(bindings.fxTypeLabel),
                                 static_cast<juce::Component*>(bindings.fxTypeSelector),
                                 static_cast<juce::Component*>(bindings.fxBypassedToggle),
                                 static_cast<juce::Component*>(bindings.fxDuplicateButton),
                                 static_cast<juce::Component*>(bindings.fxMoveUpButton),
                                 static_cast<juce::Component*>(bindings.fxMoveDownButton),
                                 static_cast<juce::Component*>(bindings.fxDeleteButton),
                                 static_cast<juce::Component*>(bindings.fxOwnerSelector),
                                 static_cast<juce::Component*>(bindings.fxMoveOwnerButton),
                                 static_cast<juce::Component*>(bindings.fxParameterSelector),
                                 static_cast<juce::Component*>(bindings.fxParameterSlider),
                                 static_cast<juce::Component*>(bindings.fxParameterResetButton),
                                 static_cast<juce::Component*>(bindings.fxAssignMacroButton),
                                 static_cast<juce::Component*>(bindings.fxParameterValueLabel) })
            setBoundsIfPresent(component, {});
        setBoundsIfPresent(bindings.fxAddButton, takeTop(area, controlRowHeight, 4));
        setBoundsIfPresent(bindings.fxDiagnosticsLabel,
                           area.removeFromBottom(std::min(20, area.getHeight())));
        area.removeFromBottom(std::min(2, area.getHeight()));
        setBoundsIfPresent(bindings.fxSummaryLabel,
                           area.removeFromBottom(std::min(18, area.getHeight())));
        fxEmptyLabel.setBounds(area.reduced(10));
        return;
    }

    fxEmptyLabel.setBounds({});
    selectedFxIdentityHeading.setBounds(takeTop(area, 17, 3));
    auto identityRow = takeTop(area, controlRowHeight, 3);
    auto typeArea = identityRow.removeFromRight(std::min(210, identityRow.getWidth() * 2 / 5));
    identityRow.removeFromRight(std::min(6, identityRow.getWidth()));
    auto bypassArea = typeArea.removeFromRight(std::min(96, typeArea.getWidth() / 2));
    typeArea.removeFromRight(std::min(6, typeArea.getWidth()));
    setBoundsIfPresent(bindings.fxNameEditor, identityRow);
    layoutLabelAndField(typeArea, bindings.fxTypeLabel, bindings.fxTypeSelector, 34);
    setBoundsIfPresent(bindings.fxBypassedToggle, bypassArea);

    auto ownerRow = takeTop(area, controlRowHeight, 3);
    auto moveOwnerArea = ownerRow.removeFromRight(std::min(130, ownerRow.getWidth() / 3));
    ownerRow.removeFromRight(std::min(6, ownerRow.getWidth()));
    setBoundsIfPresent(bindings.fxOwnerSelector, ownerRow);
    setBoundsIfPresent(bindings.fxMoveOwnerButton, moveOwnerArea);
    selectedFxContextLabel.setBounds(takeTop(area, 17, 2));

    auto parameterHeadingRow = takeTop(area, 17, 3);
    auto parameterValueArea = parameterHeadingRow.removeFromRight(
        std::min(260, parameterHeadingRow.getWidth() * 3 / 5));
    parameterHeadingRow.removeFromRight(std::min(6, parameterHeadingRow.getWidth()));
    parameterHeading.setBounds(parameterHeadingRow);
    setBoundsIfPresent(bindings.fxParameterValueLabel, parameterValueArea);

    auto parameterRow = takeTop(area, 30, 3);
    splitFields(parameterRow,
                { { bindings.fxParameterSelector, 5 },
                  { bindings.fxParameterSlider, 7 },
                  { bindings.fxParameterResetButton, 3 },
                  { bindings.fxAssignMacroButton, 5 } }, 5);
    macroControlLabel.setBounds(takeTop(area, 17, 3));

    auto actionRow = takeTop(area, controlRowHeight, 3);
    splitFields(actionRow,
                { { bindings.fxAddButton, 1 },
                  { bindings.fxDuplicateButton, 1 },
                  { bindings.fxMoveUpButton, 1 },
                  { bindings.fxMoveDownButton, 1 },
                  { bindings.fxDeleteButton, 1 } }, 5);
    setBoundsIfPresent(bindings.fxSummaryLabel, takeTop(area, 17, 2));
    setBoundsIfPresent(bindings.fxDiagnosticsLabel,
                       area.removeFromTop(std::max(1, area.getHeight())));
}
} // namespace drs::app::authoring
