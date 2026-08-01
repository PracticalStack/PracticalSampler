#include "shared/PerformanceMixer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantById(*root.getChildComponent(index), id))
            return match;
    return nullptr;
}

std::vector<drs::app::PerformanceMixerControlView> makeControls(const std::size_t count)
{
    std::vector<drs::app::PerformanceMixerControlView> controls;
    controls.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        drs::app::PerformanceMixerControlView control;
        control.authoredId = "authored-" + std::to_string(index);
        control.runtimeId = "runtime-" + std::to_string(index);
        control.sectionLabel = "Layer " + std::to_string(index + 1);
        control.controlLabel = "Control " + std::to_string(index + 1);
        control.parameterLabel = "Gain";
        control.valueUnit = index == 0 ? "dB" : "";
        control.accessibilityDescription = control.controlLabel + " in " + control.sectionLabel;
        control.controlKind = index == 0 ? drs::engine::PublishedMacroControlKind::fader
            : (index == 1 ? drs::engine::PublishedMacroControlKind::toggle
                          : drs::engine::PublishedMacroControlKind::knob);
        control.authoredOrder = index;
        control.minimum = 0.0;
        control.maximum = 1.0;
        control.value = index == 1 ? 1.0 : 0.5;
        controls.push_back(std::move(control));
    }
    return controls;
}

void requireLayout(const std::size_t count, const int width, const int expectedColumns)
{
    drs::app::PerformanceMixer mixer;
    mixer.setSize(width, 330);
    mixer.setControls(makeControls(count));
    const auto layout = mixer.getLayoutSnapshot();
    require(mixer.getControlCount() == count && layout.columnCount == expectedColumns,
            "Published mixer must select the deterministic expected column count.");

    std::vector<juce::Rectangle<int>> laneBounds;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto id = "performanceMixerControl.authored-" + juce::String(static_cast<int>(index));
        auto* lane = findDescendantById(mixer, id);
        require(lane != nullptr && lane->getWidth() > 0 && lane->getHeight() > 0,
                "Every published control must have a reachable non-empty lane.");
        for (const auto& existing : laneBounds)
            require(!existing.intersects(lane->getBounds()), "Published mixer lanes must never overlap.");
        laneBounds.push_back(lane->getBounds());
    }
    if (count == 12)
        require(layout.viewportActive, "A twelve-control mixer must expose a bounded vertical viewport.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        for (const auto count : std::array<std::size_t, 8> { 0, 1, 2, 3, 4, 5, 8, 12 })
        {
            requireLayout(count, 1000, count == 0 ? 0 : std::min(4, static_cast<int>(count)));
            requireLayout(count, 560, count == 0 ? 0 : std::min(2, static_cast<int>(count)));
        }

        std::string receivedRuntimeId;
        double receivedValue = -1.0;
        drs::app::PerformanceMixer mixer([&](const std::string& runtimeId, const double value)
        {
            receivedRuntimeId = runtimeId;
            receivedValue = value;
        });
        mixer.setSize(1000, 330);
        mixer.setControls(makeControls(3));
        auto* continuous = dynamic_cast<juce::Slider*>(findDescendantById(
            mixer, "performanceMixerWidget.authored-2"));
        require(continuous != nullptr, "Continuous controls must render a slider widget.");
        continuous->setValue(0.72, juce::sendNotificationSync);
        require(receivedRuntimeId == "runtime-2" && std::abs(receivedValue - 0.72) < 0.001,
                "A continuous widget must update its own published runtime binding.");

        auto* toggle = dynamic_cast<juce::ToggleButton*>(findDescendantById(
            mixer, "performanceMixerWidget.authored-1"));
        require(toggle != nullptr && toggle->getTitle().contains("Control 2")
                    && toggle->getDescription().contains("Layer 2"),
                "Boolean controls must render an accessible toggle with the published label.");
        toggle->setToggleState(false, juce::dontSendNotification);
        toggle->onClick();
        require(receivedRuntimeId == "runtime-1" && receivedValue == 0.0,
                "A boolean widget must update its own published runtime binding.");
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance mixer tests failed: " << exception.what() << '\n';
        return 1;
    }
}
