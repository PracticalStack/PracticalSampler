#include "shared/authoring/ZoneMappingEditor.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
void configureEditorSlider(juce::Slider& slider,
                           double minValue,
                           double maxValue,
                           double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 24);
    slider.setRange(minValue, maxValue, interval);
}

void configureFieldLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
}
} // namespace

ZoneMappingEditor::ZoneMappingEditor(LayoutMode nextLayoutMode)
    : layoutMode(nextLayoutMode)
{
    setComponentID("authoringZoneFieldEditor");

    emptyStateLabel.setComponentID("authoringZoneFieldEmptyState");
    emptyStateLabel.setJustificationType(juce::Justification::centred);
    emptyStateLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(82, 86, 94));

    configureFieldLabel(rootKeyLabel, "Root Key");
    configureFieldLabel(keyLowLabel, "Key Low");
    configureFieldLabel(keyHighLabel, "Key High");
    configureFieldLabel(velocityLowLabel, "Velocity Low");
    configureFieldLabel(velocityHighLabel, "Velocity High");
    configureFieldLabel(gainLabel, "Gain (dB)");
    configureFieldLabel(panLabel, "Pan");

    configureEditorSlider(rootKeySlider, 0, 127, 1);
    configureEditorSlider(keyLowSlider, 0, 127, 1);
    configureEditorSlider(keyHighSlider, 0, 127, 1);
    configureEditorSlider(velocityLowSlider, 1, 127, 1);
    configureEditorSlider(velocityHighSlider, 1, 127, 1);
    configureEditorSlider(gainSlider, -24.0, 12.0, 0.1);
    configureEditorSlider(panSlider, -1.0, 1.0, 0.01);

    rootKeySlider.setComponentID("authoringRootKeySlider");
    keyLowSlider.setComponentID("authoringKeyLowSlider");
    keyHighSlider.setComponentID("authoringKeyHighSlider");
    velocityLowSlider.setComponentID("authoringVelocityLowSlider");
    velocityHighSlider.setComponentID("authoringVelocityHighSlider");
    gainSlider.setComponentID("authoringGainSlider");
    panSlider.setComponentID("authoringPanSlider");
    loopEnabledToggle.setComponentID("authoringLoopEnabledToggle");
    restoreRootKeyButton.setComponentID("authoringRestoreRootKeyButton");

    loopEnabledToggle.setButtonText("Loop Enabled");
    restoreRootKeyButton.setButtonText("Restore Root Key");

    auto bindCommitOnDragEnd = [this](juce::Slider& slider, const char* labelText)
    {
        slider.onDragEnd = [this, label = juce::String(labelText)]
        {
            if (callbacks.onCommitRequested)
                callbacks.onCommitRequested(collectCurrentValues(), label.toStdString());
        };
    };

    bindCommitOnDragEnd(rootKeySlider, "Update zone root key");
    bindCommitOnDragEnd(keyLowSlider, "Update zone key range");
    bindCommitOnDragEnd(keyHighSlider, "Update zone key range");
    bindCommitOnDragEnd(velocityLowSlider, "Update zone velocity range");
    bindCommitOnDragEnd(velocityHighSlider, "Update zone velocity range");
    bindCommitOnDragEnd(gainSlider, "Update zone gain");
    bindCommitOnDragEnd(panSlider, "Update zone pan");

    loopEnabledToggle.onClick = [this]
    {
        if (callbacks.onCommitRequested)
            callbacks.onCommitRequested(collectCurrentValues(), "Toggle zone loop");
    };

    restoreRootKeyButton.onClick = [this]
    {
        if (callbacks.onRestoreRootKeyRequested)
            callbacks.onRestoreRootKeyRequested();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&emptyStateLabel),
             static_cast<juce::Component*>(&rootKeyLabel),
             static_cast<juce::Component*>(&keyLowLabel),
             static_cast<juce::Component*>(&keyHighLabel),
             static_cast<juce::Component*>(&velocityLowLabel),
             static_cast<juce::Component*>(&velocityHighLabel),
             static_cast<juce::Component*>(&gainLabel),
             static_cast<juce::Component*>(&panLabel),
             static_cast<juce::Component*>(&rootKeySlider),
             static_cast<juce::Component*>(&keyLowSlider),
             static_cast<juce::Component*>(&keyHighSlider),
             static_cast<juce::Component*>(&velocityLowSlider),
             static_cast<juce::Component*>(&velocityHighSlider),
             static_cast<juce::Component*>(&gainSlider),
             static_cast<juce::Component*>(&panSlider),
             static_cast<juce::Component*>(&loopEnabledToggle),
             static_cast<juce::Component*>(&restoreRootKeyButton)
         })
    {
        addAndMakeVisible(component);
    }
}

void ZoneMappingEditor::resized()
{
    auto area = getLocalBounds();

    if (!viewModel.hasSelection)
    {
        emptyStateLabel.setBounds(area);
        return;
    }

    constexpr int mappingColumns = 3;
    constexpr int columnGap = 12;
    constexpr int rowGap = 8;
    const auto cellHeight = layoutMode == LayoutMode::expanded ? 56 : 50;
    const auto cellWidth = (area.getWidth() - ((mappingColumns - 1) * columnGap)) / mappingColumns;

    auto getCellBounds = [&](int index)
    {
        const auto column = index % mappingColumns;
        const auto row = index / mappingColumns;
        return juce::Rectangle<int>(area.getX() + column * (cellWidth + columnGap),
                                    area.getY() + row * (cellHeight + rowGap),
                                    cellWidth,
                                    cellHeight);
    };

    auto layoutSliderCell = [&](int index, juce::Label& label, juce::Slider& slider)
    {
        auto cell = getCellBounds(index);
        label.setBounds(cell.removeFromTop(16));
        cell.removeFromTop(4);
        slider.setBounds(cell.removeFromTop(28));
    };

    auto layoutToggleCell = [&](int index, juce::ToggleButton& toggle)
    {
        auto cell = getCellBounds(index);
        cell.removeFromTop(18);
        toggle.setBounds(cell.removeFromTop(28));
    };

    layoutSliderCell(0, rootKeyLabel, rootKeySlider);
    layoutSliderCell(1, keyLowLabel, keyLowSlider);
    layoutSliderCell(2, keyHighLabel, keyHighSlider);
    layoutSliderCell(3, velocityLowLabel, velocityLowSlider);
    layoutSliderCell(4, velocityHighLabel, velocityHighSlider);
    layoutSliderCell(5, gainLabel, gainSlider);
    layoutSliderCell(6, panLabel, panSlider);
    layoutToggleCell(7, loopEnabledToggle);

    auto restoreCell = getCellBounds(8);
    restoreRootKeyButton.setBounds(restoreCell.withHeight(28)
                                              .withY(restoreCell.getCentreY() - 14));
}

void ZoneMappingEditor::setViewModel(ZoneFieldValuesViewModel nextViewModel)
{
    viewModel = std::move(nextViewModel);
    emptyStateLabel.setText(juce::String::fromUTF8(viewModel.emptyStateText.c_str()), juce::dontSendNotification);

    rootKeySlider.setValue(viewModel.rootKey, juce::dontSendNotification);
    keyLowSlider.setValue(viewModel.keyLow, juce::dontSendNotification);
    keyHighSlider.setValue(viewModel.keyHigh, juce::dontSendNotification);
    velocityLowSlider.setValue(viewModel.velocityLow, juce::dontSendNotification);
    velocityHighSlider.setValue(viewModel.velocityHigh, juce::dontSendNotification);
    gainSlider.setValue(viewModel.gainDb, juce::dontSendNotification);
    panSlider.setValue(viewModel.pan, juce::dontSendNotification);
    loopEnabledToggle.setToggleState(viewModel.loopEnabled, juce::dontSendNotification);

    emptyStateLabel.setVisible(!viewModel.hasSelection);
    for (auto* component : {
             static_cast<juce::Component*>(&rootKeyLabel),
             static_cast<juce::Component*>(&keyLowLabel),
             static_cast<juce::Component*>(&keyHighLabel),
             static_cast<juce::Component*>(&velocityLowLabel),
             static_cast<juce::Component*>(&velocityHighLabel),
             static_cast<juce::Component*>(&gainLabel),
             static_cast<juce::Component*>(&panLabel),
             static_cast<juce::Component*>(&rootKeySlider),
             static_cast<juce::Component*>(&keyLowSlider),
             static_cast<juce::Component*>(&keyHighSlider),
             static_cast<juce::Component*>(&velocityLowSlider),
             static_cast<juce::Component*>(&velocityHighSlider),
             static_cast<juce::Component*>(&gainSlider),
             static_cast<juce::Component*>(&panSlider),
             static_cast<juce::Component*>(&loopEnabledToggle),
             static_cast<juce::Component*>(&restoreRootKeyButton)
         })
    {
        component->setVisible(viewModel.hasSelection);
    }

    restoreRootKeyButton.setEnabled(viewModel.hasSelection);
    resized();
}

void ZoneMappingEditor::setCallbacks(ZoneFieldCallbacks nextCallbacks)
{
    callbacks = std::move(nextCallbacks);
}

ZoneFieldValuesViewModel ZoneMappingEditor::collectCurrentValues() const
{
    auto values = viewModel;
    values.hasSelection = true;
    values.rootKey = static_cast<int>(rootKeySlider.getValue());
    values.keyLow = static_cast<int>(keyLowSlider.getValue());
    values.keyHigh = static_cast<int>(keyHighSlider.getValue());
    values.velocityLow = static_cast<int>(velocityLowSlider.getValue());
    values.velocityHigh = static_cast<int>(velocityHighSlider.getValue());
    values.gainDb = gainSlider.getValue();
    values.pan = panSlider.getValue();
    values.loopEnabled = loopEnabledToggle.getToggleState();
    return values;
}
} // namespace drs::app::authoring
