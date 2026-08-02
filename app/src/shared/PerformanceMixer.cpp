#include "shared/PerformanceMixer.h"

#include <algorithm>
#include <cmath>

namespace drs::app
{
namespace
{
const auto mixerCardColour = juce::Colour::fromRGB(235, 240, 246);
const auto mixerTextColour = juce::Colour::fromRGB(25, 37, 53);
const auto mixerMutedTextColour = juce::Colour::fromRGB(73, 87, 105);
const auto mixerAccentColour = juce::Colour::fromRGB(28, 126, 214);

drs::engine::ControlLawUnit controlLawUnitFor(const std::string& valueUnit)
{
    if (valueUnit == "dB") return drs::engine::ControlLawUnit::decibels;
    if (valueUnit == "Hz") return drs::engine::ControlLawUnit::hertz;
    if (valueUnit == "ms") return drs::engine::ControlLawUnit::milliseconds;
    if (valueUnit == "s") return drs::engine::ControlLawUnit::seconds;
    if (valueUnit == "%") return drs::engine::ControlLawUnit::percent;
    if (valueUnit == "pan") return drs::engine::ControlLawUnit::pan;
    return drs::engine::ControlLawUnit::generic;
}

double sourceValueToNormalized(const PerformanceMixerControlView& control) noexcept
{
    const auto sourceSpan = control.maximum - control.minimum;
    return std::abs(sourceSpan) > 0.000001
        ? std::clamp((control.value - control.minimum) / sourceSpan, 0.0, 1.0) : 0.0;
}

juce::String formatValue(const PerformanceMixerControlView& control)
{
    if (control.controlKind == drs::engine::PublishedMacroControlKind::toggle)
        return control.value >= 0.5 ? "On" : "Off";

    double physical = 0.0;
    if (!drs::engine::normalizedToPhysical(control.controlLaw,
                                           sourceValueToNormalized(control), physical))
        return "Unavailable";

    drs::engine::ControlLawFormatOptions options;
    options.precision = control.valueUnit == "dB" ? 1u : 2u;
    options.renderMinimumAsNegativeInfinity
        = control.controlLaw.kind == drs::engine::ControlLawKind::mixerGainV1;
    options.negativeInfinityThreshold = control.displayMinimum;
    return juce::String::fromUTF8(drs::engine::formatControlLawValue(
        physical, controlLawUnitFor(control.valueUnit), options).c_str());
}

juce::String laneDescription(const PerformanceMixerControlView& control)
{
    auto description = juce::String::fromUTF8(control.accessibilityDescription.c_str());
    if (description.isEmpty())
        description = juce::String::fromUTF8(control.controlLabel.c_str())
            + ", " + juce::String::fromUTF8(control.sectionLabel.c_str());
    return description + ". Current value " + formatValue(control) + ".";
}
} // namespace

class PerformanceMixer::Lane final : public juce::Component
{
public:
    Lane(const PerformanceMixerControlView& initialView, ValueChangedCallback callback, const int focusOrder)
        : view(initialView), onValueChanged(std::move(callback))
    {
        setComponentID("performanceMixerControl." + juce::String::fromUTF8(view.authoredId.c_str()));
        setTitle(juce::String::fromUTF8(view.controlLabel.c_str()));
        sectionLabel.setComponentID("performanceMixerSectionLabel." + juce::String::fromUTF8(view.authoredId.c_str()));
        nameLabel.setComponentID("performanceMixerNameLabel." + juce::String::fromUTF8(view.authoredId.c_str()));
        valueLabel.setComponentID("performanceMixerValueLabel." + juce::String::fromUTF8(view.authoredId.c_str()));
        sectionLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setJustificationType(juce::Justification::centred);
        sectionLabel.setColour(juce::Label::textColourId, mixerMutedTextColour);
        nameLabel.setColour(juce::Label::textColourId, mixerTextColour);
        valueLabel.setColour(juce::Label::textColourId, mixerTextColour);
        nameLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        valueLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

        slider.setComponentID("performanceMixerWidget." + juce::String::fromUTF8(view.authoredId.c_str()));
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour(juce::Slider::trackColourId, mixerAccentColour.withAlpha(0.56f));
        slider.setColour(juce::Slider::thumbColourId, mixerAccentColour);
        slider.setWantsKeyboardFocus(true);
        slider.setExplicitFocusOrder(focusOrder);
        slider.onValueChange = [this]
        {
            if (!syncing && onValueChanged)
                onValueChanged(view.runtimeId, slider.getValue());
        };

        toggle.setComponentID("performanceMixerWidget." + juce::String::fromUTF8(view.authoredId.c_str()));
        toggle.setWantsKeyboardFocus(true);
        toggle.setExplicitFocusOrder(focusOrder);
        toggle.onClick = [this]
        {
            if (!syncing && onValueChanged)
                onValueChanged(view.runtimeId, toggle.getToggleState() ? 1.0 : 0.0);
        };

        addAndMakeVisible(sectionLabel);
        addAndMakeVisible(nameLabel);
        addAndMakeVisible(valueLabel);
        applyView(initialView);
    }

    void applyView(const PerformanceMixerControlView& next)
    {
        view = next;
        const auto description = laneDescription(view);
        auto sliderDescription = description;
        setDescription(description);
        sectionLabel.setText(juce::String::fromUTF8(view.sectionLabel.c_str()), juce::dontSendNotification);
        nameLabel.setText(juce::String::fromUTF8(view.controlLabel.c_str()), juce::dontSendNotification);
        valueLabel.setText(formatValue(view), juce::dontSendNotification);
        sectionLabel.setTooltip(sectionLabel.getText());
        nameLabel.setTooltip(nameLabel.getText());
        nameLabel.setDescription(description);
        valueLabel.setTooltip(valueLabel.getText());
        valueLabel.setDescription(description);

        syncing = true;
        const auto isToggle = view.controlKind == drs::engine::PublishedMacroControlKind::toggle;
        if (isToggle)
        {
            if (slider.getParentComponent() != nullptr)
                removeChildComponent(&slider);
            if (toggle.getParentComponent() == nullptr)
                addAndMakeVisible(toggle);
            toggle.setButtonText(view.value >= 0.5 ? "On" : "Off");
            toggle.setToggleState(view.value >= 0.5, juce::dontSendNotification);
            toggle.setTitle(juce::String::fromUTF8(view.controlLabel.c_str()));
            toggle.setDescription(description);
            toggle.setTooltip(description);
        }
        else
        {
            if (toggle.getParentComponent() != nullptr)
                removeChildComponent(&toggle);
            if (slider.getParentComponent() == nullptr)
                addAndMakeVisible(slider);
            slider.setSliderStyle(view.controlKind == drs::engine::PublishedMacroControlKind::fader
                                      ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setRange(view.minimum, view.maximum, 0.001);
            slider.setValue(view.value, juce::dontSendNotification);
            if (view.controlKind == drs::engine::PublishedMacroControlKind::fader
                && view.controlLaw.kind == drs::engine::ControlLawKind::mixerGainV1)
            {
                double unityNormalized = 0.0;
                if (drs::engine::physicalToNormalized(view.controlLaw, 0.0, unityNormalized))
                {
                    const auto sourceUnity = view.minimum + unityNormalized
                        * (view.maximum - view.minimum);
                    slider.setDoubleClickReturnValue(true, sourceUnity);
                }
                sliderDescription += " Double-click returns to unity gain; the minimum value is minus infinity.";
            }
            else
            {
                slider.setDoubleClickReturnValue(false, 0.0);
            }
            slider.setTitle(juce::String::fromUTF8(view.controlLabel.c_str()));
            slider.setDescription(sliderDescription);
            slider.setTooltip(description);
        }
        syncing = false;
        resized();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(mixerCardColour);
        graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
        if (view.controlKind == drs::engine::PublishedMacroControlKind::fader
            && view.controlLaw.kind == drs::engine::ControlLawKind::mixerGainV1)
        {
            double unityNormalized = 0.0;
            if (drs::engine::physicalToNormalized(view.controlLaw, 0.0, unityNormalized))
            {
                const auto sliderBounds = slider.getBounds();
                const auto y = static_cast<float>(sliderBounds.getBottom()
                    - unityNormalized * static_cast<double>(sliderBounds.getHeight()));
                graphics.setColour(mixerTextColour.withAlpha(0.65f));
                graphics.drawLine(static_cast<float>(sliderBounds.getX() - 5), y,
                                  static_cast<float>(sliderBounds.getRight() + 5), y, 1.5f);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(7, 6);
        sectionLabel.setBounds(area.removeFromTop(17));
        nameLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(2);
        auto footer = area.removeFromBottom(23);
        valueLabel.setBounds(footer);
        if (view.controlKind == drs::engine::PublishedMacroControlKind::toggle)
            toggle.setBounds(area.reduced(12, 12).withHeight(30).withCentre(area.getCentre()));
        else if (view.controlKind == drs::engine::PublishedMacroControlKind::fader)
            slider.setBounds(area.reduced(26, 2));
        else
            slider.setBounds(area.withSizeKeepingCentre(std::min(area.getWidth(), area.getHeight()),
                                                        std::min(area.getWidth(), area.getHeight())));
    }

private:
    PerformanceMixerControlView view;
    ValueChangedCallback onValueChanged;
    juce::Label sectionLabel;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::Slider slider;
    juce::ToggleButton toggle;
    bool syncing = false;
};

PerformanceMixer::PerformanceMixer(ValueChangedCallback callback)
    : onValueChanged(std::move(callback))
{
    setComponentID("performancePublishedMixer");
    setTitle("Published performance mixer");
    setDescription("Published player controls in authored order.");
    viewport.setComponentID("performanceMixerViewport");
    viewport.setScrollBarsShown(true, false);
    viewport.setViewedComponent(&content, false);
    addAndMakeVisible(viewport);
}

PerformanceMixer::~PerformanceMixer() = default;

void PerformanceMixer::setControls(std::vector<PerformanceMixerControlView> nextControls)
{
    std::stable_sort(nextControls.begin(), nextControls.end(), [](const auto& left, const auto& right)
    {
        return left.authoredOrder < right.authoredOrder;
    });
    const auto sameIdentity = controls.size() == nextControls.size()
        && std::equal(controls.begin(), controls.end(), nextControls.begin(), [](const auto& left, const auto& right)
        {
            return left.authoredId == right.authoredId && left.controlKind == right.controlKind;
        });
    controls = std::move(nextControls);
    if (!sameIdentity)
        rebuildLanes();
    else
        for (std::size_t index = 0; index < lanes.size(); ++index)
            lanes[index]->applyView(controls[index]);
    updateLayout();
}

std::size_t PerformanceMixer::getControlCount() const noexcept
{
    return controls.size();
}

PerformanceMixer::LayoutSnapshot PerformanceMixer::getLayoutSnapshot() const noexcept
{
    return layout;
}

void PerformanceMixer::resized()
{
    viewport.setBounds(getLocalBounds());
    updateLayout();
}

void PerformanceMixer::rebuildLanes()
{
    lanes.clear();
    content.removeAllChildren();
    lanes.reserve(controls.size());
    for (std::size_t index = 0; index < controls.size(); ++index)
    {
        auto lane = std::make_unique<Lane>(controls[index], onValueChanged, 200 + static_cast<int>(index));
        content.addAndMakeVisible(*lane);
        lanes.push_back(std::move(lane));
    }
}

void PerformanceMixer::updateLayout()
{
    layout.compact = getWidth() < 760;
    layout.columnCount = controls.empty() ? 0 : std::min(layout.compact ? 2 : 4, static_cast<int>(controls.size()));
    layout.rowCount = layout.columnCount == 0 ? 0
        : static_cast<int>((controls.size() + static_cast<std::size_t>(layout.columnCount) - 1)
                           / static_cast<std::size_t>(layout.columnCount));
    constexpr int gap = 10;
    constexpr int laneHeight = 174;
    const auto contentWidth = std::max(1, viewport.getMaximumVisibleWidth());
    const auto contentHeight = layout.rowCount == 0 ? 1 : layout.rowCount * laneHeight
        + (layout.rowCount - 1) * gap;
    layout.viewportActive = contentHeight > viewport.getMaximumVisibleHeight();
    content.setSize(contentWidth, contentHeight);
    if (layout.columnCount == 0)
        return;

    const auto laneWidth = std::max(1, (contentWidth - (layout.columnCount - 1) * gap) / layout.columnCount);
    for (std::size_t index = 0; index < lanes.size(); ++index)
    {
        const auto row = static_cast<int>(index) / layout.columnCount;
        const auto column = static_cast<int>(index) % layout.columnCount;
        lanes[index]->setBounds(column * (laneWidth + gap), row * (laneHeight + gap), laneWidth, laneHeight);
    }
}
} // namespace drs::app
