#include "shared/InstrumentControlsPanel.h"

#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <cmath>

namespace drs::app
{
namespace
{
constexpr auto rowHeight = 58;
constexpr auto contentPadding = 16;

juce::String displayName(const drs::engine::EngineInstrumentControlDescriptor& descriptor)
{
    const auto name = juce::String::fromUTF8(descriptor.name.c_str()).trim();
    return name.isNotEmpty() ? name : juce::String::fromUTF8(descriptor.id.c_str());
}
} // namespace

InstrumentControlsPanel::ControlRow::ControlRow(
    const drs::engine::EngineInstrumentControlDescriptor& nextDescriptor)
    : descriptor(nextDescriptor)
{
    setComponentID("instrumentControlRow." + juce::String::fromUTF8(descriptor.id.c_str()));
    nameLabel.setText(displayName(descriptor), juce::dontSendNotification);
    nameLabel.setColour(juce::Label::textColourId, authoring::visual::text);
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::FontOptions(14.0f, juce::Font::plain));

    valueLabel.setColour(juce::Label::textColourId, authoring::visual::textMuted);
    valueLabel.setJustificationType(juce::Justification::centredRight);
    valueLabel.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    valueLabel.setText(formatValue(descriptor), juce::dontSendNotification);

    slider.setComponentID("instrumentControlSlider." + juce::String::fromUTF8(descriptor.id.c_str()));
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(0.0, 1.0, descriptor.kind == drs::engine::RuntimeInstrumentControlKind::stepped
        ? 0.01 : 0.001);
    slider.setValue(descriptor.currentValue, juce::dontSendNotification);
    slider.setTooltip(displayName(descriptor));

    addAndMakeVisible(nameLabel);
    addAndMakeVisible(slider);
    addAndMakeVisible(valueLabel);
}

void InstrumentControlsPanel::ControlRow::resized()
{
    auto area = getLocalBounds().reduced(4, 7);
    nameLabel.setBounds(area.removeFromLeft(std::min(190, std::max(100, area.getWidth() / 3))));
    area.removeFromLeft(14);
    valueLabel.setBounds(area.removeFromRight(64));
    area.removeFromRight(10);
    slider.setBounds(area);
}

void InstrumentControlsPanel::ControlRow::update(
    const drs::engine::EngineInstrumentControlDescriptor& nextDescriptor,
    const bool sendNotification)
{
    descriptor = nextDescriptor;
    nameLabel.setText(displayName(descriptor), juce::dontSendNotification);
    slider.setRange(0.0, 1.0,
                    descriptor.kind == drs::engine::RuntimeInstrumentControlKind::stepped
                        ? 0.01 : 0.001);
    slider.setValue(std::clamp(descriptor.currentValue, 0.0, 1.0),
                    sendNotification ? juce::sendNotification : juce::dontSendNotification);
    valueLabel.setText(formatValue(descriptor), juce::dontSendNotification);
}

juce::String InstrumentControlsPanel::ControlRow::formatValue(
    const drs::engine::EngineInstrumentControlDescriptor& descriptor)
{
    const auto value = std::clamp(descriptor.currentValue, 0.0, 1.0);
    if (descriptor.kind == drs::engine::RuntimeInstrumentControlKind::toggle
        || descriptor.unit == drs::engine::RuntimeInstrumentControlUnit::boolean)
        return value >= 0.5 ? "On" : "Off";

    if (descriptor.unit == drs::engine::RuntimeInstrumentControlUnit::percent
        || descriptor.kind == drs::engine::RuntimeInstrumentControlKind::percent)
        return juce::String(value * 100.0, 1) + "%";

    return juce::String(value * 100.0, 1) + "%";
}

InstrumentControlsPanel::InstrumentControlsPanel(
    drs::engine::EngineFacade& nextEngineFacade,
    ValueChangedCallback nextValueChangedCallback,
    DescriptorProvider nextDescriptorProvider)
    : engineFacade(nextEngineFacade),
      valueChangedCallback(std::move(nextValueChangedCallback)),
      descriptorProvider(std::move(nextDescriptorProvider))
{
    setComponentID("instrumentControlsPanel");
    viewport.setComponentID("instrumentControlsViewport");
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(12);
    viewport.setViewedComponent(&content, false);
    addAndMakeVisible(viewport);
    refreshNow();
}

void InstrumentControlsPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoring::visual::shell);
    auto area = getLocalBounds().reduced(18, 16);
    g.setColour(authoring::visual::text);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("Controls", area.removeFromTop(26), juce::Justification::centredLeft);
    g.setColour(authoring::visual::textMuted);
    g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    g.drawText("Instrument controls", area.removeFromTop(20), juce::Justification::centredLeft);
}

void InstrumentControlsPanel::resized()
{
    viewport.setBounds(getLocalBounds().reduced(14, 62));
    const auto contentWidth = std::max(240, viewport.getWidth() - viewport.getScrollBarThickness());
    content.setSize(contentWidth,
                    contentPadding * 2 + static_cast<int>(rows.size()) * rowHeight);
    auto area = content.getLocalBounds().reduced(contentPadding, contentPadding);
    for (auto& row : rows)
        row->setBounds(area.removeFromTop(rowHeight));
}

bool InstrumentControlsPanel::sameTopology(
    const std::vector<std::unique_ptr<ControlRow>>& currentRows,
    const std::vector<drs::engine::EngineInstrumentControlDescriptor>& descriptors)
{
    if (currentRows.size() != descriptors.size())
        return false;
    return std::equal(currentRows.begin(), currentRows.end(), descriptors.begin(),
                      [](const auto& row, const auto& descriptor)
                      {
                          return row->descriptor.id == descriptor.id;
                      });
}

void InstrumentControlsPanel::rebuild(
    const std::vector<drs::engine::EngineInstrumentControlDescriptor>& descriptors)
{
    rows.clear();
    for (const auto& descriptor : descriptors)
    {
        auto row = std::make_unique<ControlRow>(descriptor);
        const auto controlId = descriptor.id;
        row->slider.onValueChange = [this, controlId]
        {
            if (refreshing || !valueChangedCallback)
                return;
            auto value = 0.0;
            for (const auto& candidate : rows)
            {
                if (candidate->descriptor.id == controlId)
                {
                    value = candidate->slider.getValue();
                    break;
                }
            }
            valueChangedCallback(controlId, value);
        };
        content.addAndMakeVisible(*row);
        rows.push_back(std::move(row));
    }
    resized();
}

void InstrumentControlsPanel::refreshNow()
{
    const auto runtimeDescriptors = engineFacade.getInstrumentControlDescriptors();
    auto descriptors = descriptorProvider ? descriptorProvider() : runtimeDescriptors;
    if (descriptors.empty())
        descriptors = runtimeDescriptors;

    for (auto& descriptor : descriptors)
    {
        const auto runtime = std::find_if(runtimeDescriptors.begin(), runtimeDescriptors.end(),
            [&](const auto& candidate) { return candidate.id == descriptor.id; });
        if (runtime != runtimeDescriptors.end())
        {
            descriptor.currentValue = runtime->currentValue;
            descriptor.assignedControllers = runtime->assignedControllers;
        }
    }

    std::vector<drs::engine::EngineInstrumentControlDescriptor> visibleDescriptors;
    for (const auto& descriptor : descriptors)
        if (descriptor.visible && !descriptor.id.empty())
            visibleDescriptors.push_back(descriptor);

    if (!sameTopology(rows, visibleDescriptors))
    {
        rebuild(visibleDescriptors);
        repaint();
        return;
    }

    const juce::ScopedValueSetter<bool> guard(refreshing, true);
    for (std::size_t index = 0; index < rows.size(); ++index)
        rows[index]->update(visibleDescriptors[index], false);
}
} // namespace drs::app
