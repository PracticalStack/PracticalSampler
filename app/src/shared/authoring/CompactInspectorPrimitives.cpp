#include "shared/authoring/CompactInspectorPrimitives.h"

namespace drs::app::authoring
{
namespace
{
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

void setAccessibleRecursively(juce::Component& component, bool shouldExpose)
{
    component.setAccessible(shouldExpose);

    for (int childIndex = 0; childIndex < component.getNumChildComponents(); ++childIndex)
        setAccessibleRecursively(*component.getChildComponent(childIndex), shouldExpose);
}

void setVisibleAndAccessible(juce::Component& component, bool shouldShow)
{
    component.setVisible(shouldShow);
    setAccessibleRecursively(component, shouldShow);
}

bool isFocusedWithin(const juce::Component* focusedComponent, const juce::Component& ancestor)
{
    for (auto* current = focusedComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current == &ancestor)
            return true;
    }

    return false;
}

void configureInspectorLabel(juce::Label& label,
                             const juce::String& text,
                             float fontHeight,
                             bool bold)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(fontHeight, bold ? juce::Font::bold : juce::Font::plain));
}

void configureInspectorSlider(juce::Slider& slider,
                              double minValue,
                              double maxValue,
                              double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 22);
    slider.setRange(minValue, maxValue, interval);
}
} // namespace

CompactInspectorCommitSlider::CompactInspectorCommitSlider()
{
    onDragStart = [this]
    {
        attachTextBoxListener();

        if (!textEditActive)
            pointerGestureActive = true;
    };

    onDragEnd = [this]
    {
        if (!pointerGestureActive)
            return;

        pointerGestureActive = false;

        if (onCommitFinished)
            onCommitFinished(CommitSource::pointerDrag);
    };
}

CompactInspectorCommitSlider::~CompactInspectorCommitSlider()
{
    detachTextBoxListener();
}

void CompactInspectorCommitSlider::lookAndFeelChanged()
{
    juce::Slider::lookAndFeelChanged();
    attachTextBoxListener();
}

void CompactInspectorCommitSlider::resized()
{
    juce::Slider::resized();
    attachTextBoxListener();
}

void CompactInspectorCommitSlider::setOnCommitFinished(std::function<void(CommitSource)> nextCallback)
{
    onCommitFinished = std::move(nextCallback);
}

void CompactInspectorCommitSlider::labelTextChanged(juce::Label*)
{
}

void CompactInspectorCommitSlider::editorShown(juce::Label*, juce::TextEditor&)
{
    textEditActive = true;
    textEditStartValue = getValue();
}

void CompactInspectorCommitSlider::editorHidden(juce::Label*, juce::TextEditor&)
{
    const auto changed = !juce::approximatelyEqual(textEditStartValue, getValue());
    textEditActive = false;
    pointerGestureActive = false;

    if (changed && onCommitFinished)
        onCommitFinished(CommitSource::textEntry);
}

void CompactInspectorCommitSlider::attachTextBoxListener()
{
    juce::Label* nextTextBoxLabel = nullptr;

    for (int childIndex = 0; childIndex < getNumChildComponents(); ++childIndex)
    {
        nextTextBoxLabel = dynamic_cast<juce::Label*>(getChildComponent(childIndex));
        if (nextTextBoxLabel != nullptr)
            break;
    }

    if (textBoxLabel == nextTextBoxLabel)
        return;

    detachTextBoxListener();
    textBoxLabel = nextTextBoxLabel;

    if (textBoxLabel != nullptr)
        textBoxLabel->addListener(this);
}

void CompactInspectorCommitSlider::detachTextBoxListener()
{
    if (textBoxLabel != nullptr)
        textBoxLabel->removeListener(this);

    textBoxLabel = nullptr;
}

CompactInspectorSection::CompactInspectorSection(const juce::String& titleText,
                                                 const juce::String& sectionComponentId,
                                                 bool startsExpanded)
    : sectionTitle(titleText),
      expanded(startsExpanded)
{
    setComponentID(sectionComponentId);

    configureInspectorLabel(titleLabel, titleText, 13.0f, true);
    configureAccessibleMetadata(*this,
                                titleText + " inspector section",
                                "Groups related " + titleText.toLowerCase() + " controls.");
    configureAccessibleMetadata(titleLabel,
                                titleText,
                                "Inspector section heading.");

    disclosureButton.setClickingTogglesState(true);
    disclosureButton.setComponentID(sectionComponentId + "Disclosure");
    disclosureButton.setToggleState(expanded, juce::dontSendNotification);
    disclosureButton.setButtonText(expanded ? "Hide" : "Show");
    configureAccessibleMetadata(disclosureButton,
                                (expanded ? "Hide " : "Show ") + titleText + " section",
                                "Shows or hides the " + titleText.toLowerCase() + " inspector section.",
                                "Press to expand or collapse this section.");
    disclosureButton.onClick = [this]
    {
        setExpanded(!expanded);
    };

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(disclosureButton);
}

void CompactInspectorSection::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(24);
    titleLabel.setBounds(header.removeFromLeft(header.getWidth() - 72));
    disclosureButton.setBounds(header.removeFromRight(72));

    if (content != nullptr)
    {
        setVisibleAndAccessible(*content, expanded);
        content->setBounds(area);
    }
}

void CompactInspectorSection::setContent(juce::Component* nextContent)
{
    content = nextContent;
    if (content != nullptr)
    {
        addAndMakeVisible(content);
        expandedContentHeight = content->getHeight();
        configureAccessibleMetadata(*content,
                                    sectionTitle + " section content",
                                    "Contains editable " + sectionTitle.toLowerCase() + " inspector controls.");
        setVisibleAndAccessible(*content, expanded);
    }

    resized();
}

void CompactInspectorSection::setExpanded(bool shouldExpand)
{
    if (expanded == shouldExpand)
        return;

    const auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusInsideContent = content != nullptr && isFocusedWithin(focusedComponent, *content);

    expanded = shouldExpand;
    disclosureButton.setToggleState(expanded, juce::dontSendNotification);
    disclosureButton.setButtonText(expanded ? "Hide" : "Show");
    disclosureButton.setTitle((expanded ? "Hide " : "Show ") + sectionTitle + " section");
    if (content != nullptr)
        setVisibleAndAccessible(*content, expanded);
    if (!expanded && focusInsideContent)
        disclosureButton.grabKeyboardFocus();
    if (onExpandedChanged)
        onExpandedChanged(expanded);
    resized();
}

int CompactInspectorSection::getPreferredHeight() const
{
    return 24 + (expanded ? expandedContentHeight : 0);
}

void CompactInspectorSection::setOnExpandedChanged(std::function<void(bool)> nextCallback)
{
    onExpandedChanged = std::move(nextCallback);
}

CompactInspectorSliderRow::CompactInspectorSliderRow(const juce::String& labelText,
                                                     const juce::String& rowComponentId,
                                                     double minValue,
                                                     double maxValue,
                                                     double interval)
{
    setComponentID(rowComponentId);
    configureInspectorLabel(label, labelText, 12.5f, true);
    configureInspectorSlider(slider, minValue, maxValue, interval);
    configureAccessibleMetadata(label,
                                labelText,
                                "Inspector field label.");
    configureAccessibleMetadata(slider,
                                labelText,
                                "Adjusts " + labelText.toLowerCase() + ".",
                                "Drag the slider or enter a numeric value.");
    addAndMakeVisible(label);
    addAndMakeVisible(slider);
}

void CompactInspectorSliderRow::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);
    slider.setBounds(area.removeFromTop(24));
}

CompactInspectorRangeRow::CompactInspectorRangeRow(const juce::String& titleText,
                                                   const juce::String& rowComponentId,
                                                   const juce::String& lowLabelText,
                                                   const juce::String& highLabelText,
                                                   double minValue,
                                                   double maxValue,
                                                   double interval)
{
    setComponentID(rowComponentId);
    configureInspectorLabel(titleLabel, titleText, 12.5f, true);
    configureInspectorLabel(lowLabel, lowLabelText, 11.5f, false);
    configureInspectorLabel(highLabel, highLabelText, 11.5f, false);
    configureInspectorSlider(lowSlider, minValue, maxValue, interval);
    configureInspectorSlider(highSlider, minValue, maxValue, interval);
    configureAccessibleMetadata(titleLabel,
                                titleText,
                                "Inspector field label.");
    configureAccessibleMetadata(lowLabel,
                                titleText + " low label",
                                "Low value label for " + titleText.toLowerCase() + ".");
    configureAccessibleMetadata(highLabel,
                                titleText + " high label",
                                "High value label for " + titleText.toLowerCase() + ".");
    configureAccessibleMetadata(lowSlider,
                                titleText + " low",
                                "Adjusts the low value for " + titleText.toLowerCase() + ".",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(highSlider,
                                titleText + " high",
                                "Adjusts the high value for " + titleText.toLowerCase() + ".",
                                "Drag the slider or enter a numeric value.");

    addAndMakeVisible(titleLabel);
    addAndMakeVisible(lowLabel);
    addAndMakeVisible(highLabel);
    addAndMakeVisible(lowSlider);
    addAndMakeVisible(highSlider);
}

void CompactInspectorRangeRow::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(4);

    auto top = area.removeFromTop(14);
    auto sliders = area.removeFromTop(24);
    auto leftTop = top.removeFromLeft((top.getWidth() - 10) / 2);
    top.removeFromLeft(10);
    auto rightTop = top;
    lowLabel.setBounds(leftTop);
    highLabel.setBounds(rightTop);

    auto leftSlider = sliders.removeFromLeft((sliders.getWidth() - 10) / 2);
    sliders.removeFromLeft(10);
    auto rightSlider = sliders;
    lowSlider.setBounds(leftSlider);
    highSlider.setBounds(rightSlider);
}

CompactInspectorToggleRow::CompactInspectorToggleRow(const juce::String& titleText,
                                                     const juce::String& rowComponentId,
                                                     const juce::String& toggleText)
{
    setComponentID(rowComponentId);
    configureInspectorLabel(label, titleText, 12.5f, true);
    toggle.setButtonText(toggleText);
    configureAccessibleMetadata(label,
                                titleText,
                                "Inspector field label.");
    configureAccessibleMetadata(toggle,
                                titleText,
                                "Toggles the " + titleText.toLowerCase() + " state.",
                                "Press to turn this setting on or off.");
    addAndMakeVisible(label);
    addAndMakeVisible(toggle);
}

void CompactInspectorToggleRow::resized()
{
    auto area = getLocalBounds();
    auto row = area.removeFromTop(24);
    label.setBounds(row.removeFromLeft(row.getWidth() - 130));
    toggle.setBounds(row);
}

CompactInspectorActionRow::CompactInspectorActionRow(const juce::String& titleText,
                                                     const juce::String& rowComponentId,
                                                     const juce::String& buttonText)
{
    setComponentID(rowComponentId);
    configureInspectorLabel(label, titleText, 12.5f, true);
    button.setButtonText(buttonText);
    configureAccessibleMetadata(label,
                                titleText,
                                "Inspector action label.");
    configureAccessibleMetadata(button,
                                buttonText,
                                "Runs the " + titleText.toLowerCase() + " action.",
                                "Press to apply this action.");
    addAndMakeVisible(label);
    addAndMakeVisible(button);
}

void CompactInspectorActionRow::resized()
{
    auto area = getLocalBounds();
    auto row = area.removeFromTop(24);
    label.setBounds(row.removeFromLeft(row.getWidth() - 150));
    button.setBounds(row);
}

CompactInspectorMessage::CompactInspectorMessage(const juce::String& messageComponentId,
                                                 juce::Justification justification)
{
    setComponentID(messageComponentId);
    label.setJustificationType(justification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(82, 86, 94));
    label.setFont(juce::FontOptions(12.5f, juce::Font::plain));
    configureAccessibleMetadata(*this,
                                "Inspector message",
                                "Displays compact inspector guidance or validation feedback.");
    addAndMakeVisible(label);
}

void CompactInspectorMessage::resized()
{
    label.setBounds(getLocalBounds());
}

void CompactInspectorMessage::setText(const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    configureAccessibleMetadata(label,
                                text,
                                "Inspector message text.");
}
} // namespace drs::app::authoring
