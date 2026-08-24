#include "shared/authoring/InstrumentControlsWorkbenchView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <algorithm>
#include <unordered_map>

namespace drs::app::authoring
{
namespace
{
class ResettableSlider final : public juce::Slider
{
public:
    std::function<void()> onReset;

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        juce::Slider::mouseDoubleClick(event);
        if (onReset != nullptr)
            onReset();
    }
};

juce::String categoryName(const drs::engine::RuntimeInstrumentControlCategory category)
{
    switch (category)
    {
        case drs::engine::RuntimeInstrumentControlCategory::mixer: return "Mixer";
        case drs::engine::RuntimeInstrumentControlCategory::tuning: return "Tuning";
        case drs::engine::RuntimeInstrumentControlCategory::envelope: return "Envelope";
        case drs::engine::RuntimeInstrumentControlCategory::dynamics: return "Dynamics";
        case drs::engine::RuntimeInstrumentControlCategory::tone: return "Tone";
        case drs::engine::RuntimeInstrumentControlCategory::hidden: return "Imported";
    }
    return "Imported";
}

juce::String unitSuffix(const drs::engine::RuntimeInstrumentControlUnit unit)
{
    switch (unit)
    {
        case drs::engine::RuntimeInstrumentControlUnit::decibels: return " dB";
        case drs::engine::RuntimeInstrumentControlUnit::pan: return " pan";
        case drs::engine::RuntimeInstrumentControlUnit::cents: return " cents";
        case drs::engine::RuntimeInstrumentControlUnit::seconds: return " s";
        case drs::engine::RuntimeInstrumentControlUnit::percent: return "%";
        case drs::engine::RuntimeInstrumentControlUnit::integer: return "";
        case drs::engine::RuntimeInstrumentControlUnit::boolean: return "";
        case drs::engine::RuntimeInstrumentControlUnit::generic: break;
    }
    return "";
}
}

InstrumentControlsWorkbenchView::InstrumentControlsWorkbenchView()
{
    setComponentID("authoringInstrumentControlsContent");
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    filterEditor.setComponentID("instrumentControlsFilter");
    filterEditor.setAccessible(true);
    filterEditor.setWantsKeyboardFocus(true);
    filterEditor.setTitle("Filter instrument controls");
    filterEditor.setTextToShowWhenEmpty("Filter controls or MIDI sources", juce::Colour(0xff687278));
    filterEditor.setTooltip("Filter by control name, category, or MIDI source");
    filterEditor.onTextChange = [this]
    {
        if (!refreshing)
        {
            resized();
            repaint();
        }
    };
    addAndMakeVisible(filterEditor);
    sortSelector.setComponentID("instrumentControlsSort");
    sortSelector.setAccessible(true);
    sortSelector.setWantsKeyboardFocus(true);
    sortSelector.setTitle("Sort instrument controls");
    sortSelector.addItem("Display order", 1);
    sortSelector.addItem("Name", 2);
    sortSelector.addItem("MIDI source", 3);
    sortSelector.setSelectedId(1, juce::dontSendNotification);
    sortSelector.setTooltip("Sort the control and assignment cards");
    sortSelector.onChange = [this]
    {
        if (!refreshing)
            setControls(sourceControls, sourceBindings);
    };
    mixerSurfaceButton.setComponentID("instrumentControlsMixerSurface");
    mixerSurfaceButton.setAccessible(true);
    mixerSurfaceButton.setWantsKeyboardFocus(true);
    mixerSurfaceButton.setTitle("Show mixer controls");
    mixerSurfaceButton.setTooltip("Show master, bus, and kit-piece gain and pan controls");
    mixerSurfaceButton.setClickingTogglesState(false);
    mixerSurfaceButton.onClick = [this] { setSurface(Surface::mixer); };
    instrumentSurfaceButton.setComponentID("instrumentControlsParameterSurface");
    instrumentSurfaceButton.setAccessible(true);
    instrumentSurfaceButton.setWantsKeyboardFocus(true);
    instrumentSurfaceButton.setTitle("Show instrument parameter controls");
    instrumentSurfaceButton.setTooltip("Show tuning, envelope, dynamics, and tone controls");
    instrumentSurfaceButton.setClickingTogglesState(false);
    instrumentSurfaceButton.onClick = [this] { setSurface(Surface::instrumentControls); };
    assignmentsDrawerButton.setComponentID("instrumentControlsAssignmentsDrawerToggle");
    assignmentsDrawerButton.setAccessible(true);
    assignmentsDrawerButton.setWantsKeyboardFocus(true);
    assignmentsDrawerButton.setTitle("Open MIDI assignments drawer");
    assignmentsDrawerButton.setTooltip("Show only MIDI assignments in a full-height drawer");
    assignmentsDrawerButton.onClick = [this]
    {
        setAssignmentsDrawerOpen(!assignmentsDrawerOpen);
    };
    addAndMakeVisible(assignmentsDrawerButton);
    addAndMakeVisible(mixerSurfaceButton);
    addAndMakeVisible(instrumentSurfaceButton);
    mixerSurfaceButton.setToggleState(false, juce::dontSendNotification);
    instrumentSurfaceButton.setToggleState(true, juce::dontSendNotification);
}

void InstrumentControlsWorkbenchView::setAssignmentsDrawerOpen(const bool open)
{
    if (assignmentsDrawerOpen == open)
        return;
    assignmentsDrawerOpen = open;
    assignmentsDrawerButton.setButtonText(assignmentsDrawerOpen ? "Controls" : "Assignments");
    assignmentsDrawerButton.setTitle(assignmentsDrawerOpen
                                          ? "Return to instrument controls"
                                          : "Open MIDI assignments drawer");
    resized();
    repaint();
}

void InstrumentControlsWorkbenchView::setSurface(const Surface surfaceToShow)
{
    if (activeSurface == surfaceToShow)
        return;
    activeSurface = surfaceToShow;
    mixerSurfaceButton.setToggleState(activeSurface == Surface::mixer,
                                      juce::dontSendNotification);
    instrumentSurfaceButton.setToggleState(activeSurface == Surface::instrumentControls,
                                            juce::dontSendNotification);
    resized();
    repaint();
}

void InstrumentControlsWorkbenchView::setControls(
    std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition> controls,
    std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition> bindings)
{
    sourceControls = controls;
    sourceBindings = bindings;
    midiSourceStatuses.clear();
    std::unordered_map<std::string, double> previousValues;
    for (std::size_t index = 0; index < rows.size() && index < sliders.size(); ++index)
        previousValues.emplace(rows[index].id, sliders[index]->getValue());
    rows.clear();
    sliders.clear();
    controllerSelectors.clear();
    channelSelectors.clear();
    learnButtons.clear();
    clearButtons.clear();
    restoreButtons.clear();
    replaceButtons.clear();
    cancelConflictButtons.clear();
    pendingBindings.clear();
    refreshing = true;
    std::sort(controls.begin(), controls.end(), [](const auto& left, const auto& right)
    {
        return left.displayOrder < right.displayOrder;
    });
    if (sortSelector.getSelectedId() == 2)
    {
        std::stable_sort(controls.begin(), controls.end(), [](const auto& left, const auto& right)
        {
            return left.displayName < right.displayName;
        });
    }
    else if (sortSelector.getSelectedId() == 3)
    {
        std::stable_sort(controls.begin(), controls.end(), [&](const auto& left, const auto& right)
        {
            const auto sourceFor = [&](const auto& control)
            {
                const auto binding = std::find_if(bindings.begin(), bindings.end(), [&](const auto& candidate)
                {
                    return candidate.controlId == control.id && candidate.enabled;
                });
                return binding == bindings.end() ? 999 : binding->controllerNumber;
            };
            const auto leftSource = sourceFor(left);
            const auto rightSource = sourceFor(right);
            return leftSource == rightSource ? left.displayOrder < right.displayOrder
                                             : leftSource < rightSource;
        });
    }
    for (const auto& control : controls)
    {
        if (!control.visible)
            continue;
        juce::String detail = categoryName(control.category);
        if (control.importedSourceController.has_value())
            detail << "  ·  CC " << *control.importedSourceController;
        const auto binding = std::find_if(bindings.begin(), bindings.end(), [&](const auto& candidate)
        {
            return candidate.controlId == control.id && candidate.enabled;
        });
        if (binding != bindings.end())
            detail << "  ·  MIDI " << binding->controllerNumber;
        if (binding != bindings.end())
        {
            midiSourceStatuses[control.id] = "Awaiting MIDI input";
            detail << "  ·  Awaiting MIDI input";
        }
        const auto previous = previousValues.find(control.id);
        const auto currentValue = previous == previousValues.end()
            ? control.normalizedDefault : previous->second;
        rows.push_back({ juce::String::fromUTF8(control.displayName.c_str()), detail, control.category,
                         control.id, currentValue, control.displayMinimum, control.displayMaximum,
                         control.displayPrecision, control.unit });
        pendingBindings.push_back({});
        const auto rowIndex = rows.size() - 1;
        auto slider = std::make_unique<ResettableSlider>();
        slider->setAccessible(true);
        slider->setWantsKeyboardFocus(true);
        slider->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".value");
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setRange(0.0, 1.0, 0.001);
        slider->setValue(currentValue, juce::dontSendNotification);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
        slider->textFromValueFunction = [minimum = control.displayMinimum,
                                         maximum = control.displayMaximum,
                                         precision = control.displayPrecision,
                                         unit = control.unit](const double normalized)
        {
            const auto physical = minimum + (maximum - minimum) * juce::jlimit(0.0, 1.0, normalized);
            return juce::String(physical, juce::jlimit(0, 6, precision)) + unitSuffix(unit);
        };
        const auto id = control.id;
        auto* sliderPtr = slider.get();
        slider->setTitle(juce::String::fromUTF8(control.displayName.c_str()));
        slider->onValueChange = [this, id, sliderPtr]
        {
            if (!refreshing && valueChangedCallback != nullptr)
                valueChangedCallback(id, sliderPtr->getValue());
        };
        slider->onReset = [this, id, sliderPtr, defaultValue = control.normalizedDefault]
        {
            if (resetCallback != nullptr)
                resetCallback(id);
            refreshing = true;
            sliderPtr->setValue(defaultValue, juce::sendNotificationSync);
            refreshing = false;
        };
        sliders.push_back(std::move(slider));

        auto controllerSelector = std::make_unique<juce::ComboBox>();
        controllerSelector->setAccessible(true);
        controllerSelector->setWantsKeyboardFocus(true);
        controllerSelector->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".controller");
        controllerSelector->setTitle("MIDI CC source for " + juce::String::fromUTF8(control.displayName.c_str()));
        controllerSelector->setTooltip("Choose the MIDI CC source for this control");
        controllerSelector->addItem("Unassigned", 1);
        for (int controller = 0; controller <= 127; ++controller)
            controllerSelector->addItem("CC " + juce::String(controller), controller + 2);
        controllerSelector->setSelectedId(binding == bindings.end()
                                               ? 1 : binding->controllerNumber + 2,
                                           juce::dontSendNotification);
        auto channelSelector = std::make_unique<juce::ComboBox>();
        channelSelector->setAccessible(true);
        channelSelector->setWantsKeyboardFocus(true);
        channelSelector->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".channel");
        channelSelector->setTitle("MIDI channel scope for " + juce::String::fromUTF8(control.displayName.c_str()));
        channelSelector->setTooltip("Choose Any channel or a specific MIDI channel");
        channelSelector->addItem("Any", 1);
        for (int channel = 1; channel <= 16; ++channel)
            channelSelector->addItem("Ch " + juce::String(channel), channel + 1);
        const auto channelItem = binding != bindings.end()
            && binding->channelScope.kind == drs::engine::RuntimeMidiChannelScopeKind::exact
            ? static_cast<int>(binding->channelScope.channel) + 1 : 1;
        channelSelector->setSelectedId(channelItem, juce::dontSendNotification);
        auto* controllerSelectorPtr = controllerSelector.get();
        auto* channelSelectorPtr = channelSelector.get();
        controllerSelector->onChange = [this, rowIndex, controllerSelectorPtr, channelSelectorPtr]
        {
            if (refreshing)
                return;
            const auto selected = controllerSelectorPtr->getSelectedId();
            const auto channel = channelSelectorPtr->getSelectedId();
            requestBindingChange(rowIndex, selected <= 1 ? -1 : selected - 2,
                                 channel <= 1 ? std::uint8_t { 0 }
                                              : static_cast<std::uint8_t>(channel - 1),
                                 *controllerSelectorPtr, *channelSelectorPtr);
        };
        channelSelector->onChange = [this, rowIndex, controllerSelectorPtr, channelSelectorPtr]
        {
            if (refreshing)
                return;
            const auto selected = controllerSelectorPtr->getSelectedId();
            const auto channel = channelSelectorPtr->getSelectedId();
            requestBindingChange(rowIndex, selected <= 1 ? -1 : selected - 2,
                                 channel <= 1 ? std::uint8_t { 0 }
                                              : static_cast<std::uint8_t>(channel - 1),
                                 *controllerSelectorPtr, *channelSelectorPtr);
        };
        controllerSelectors.push_back(std::move(controllerSelector));
        channelSelectors.push_back(std::move(channelSelector));

        auto learnButton = std::make_unique<juce::TextButton>("Learn");
        learnButton->setAccessible(true);
        learnButton->setWantsKeyboardFocus(true);
        learnButton->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".learn");
        learnButton->setTitle("Learn MIDI source for " + juce::String::fromUTF8(control.displayName.c_str()));
        learnButton->setTooltip("Listen for the next non-reserved MIDI CC");
        learnButton->onClick = [this, id]
        {
            armMidiLearn(id, static_cast<std::uint64_t>(juce::Time::currentTimeMillis()));
        };
        learnButtons.push_back(std::move(learnButton));
        auto clearButton = std::make_unique<juce::TextButton>("×");
        clearButton->setAccessible(true);
        clearButton->setWantsKeyboardFocus(true);
        clearButton->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".clear");
        clearButton->setTitle("Clear MIDI source for " + juce::String::fromUTF8(control.displayName.c_str()));
        clearButton->setTooltip("Clear MIDI assignment");
        clearButton->onClick = [this, id]
        {
            clearMidiAssignment(id);
        };
        clearButtons.push_back(std::move(clearButton));
        auto restoreButton = std::make_unique<juce::TextButton>("R");
        restoreButton->setAccessible(true);
        restoreButton->setWantsKeyboardFocus(true);
        restoreButton->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".restore");
        restoreButton->setTitle("Restore imported MIDI source for " + juce::String::fromUTF8(control.displayName.c_str()));
        restoreButton->setTooltip("Restore the imported MIDI source");
        restoreButton->onClick = [this, id]
        {
            restoreMidiAssignment(id);
        };
        restoreButtons.push_back(std::move(restoreButton));

        auto replaceButton = std::make_unique<juce::TextButton>("Replace");
        replaceButton->setAccessible(true);
        replaceButton->setWantsKeyboardFocus(true);
        replaceButton->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".replace");
        replaceButton->setTitle("Replace conflicting MIDI source for " + juce::String::fromUTF8(control.displayName.c_str()));
        replaceButton->setTooltip("Replace the occupied MIDI source after reviewing the conflict");
        replaceButton->setVisible(false);
        replaceButton->onClick = [this, rowIndex] { applyPendingBinding(rowIndex); };
        replaceButtons.push_back(std::move(replaceButton));
        auto cancelConflictButton = std::make_unique<juce::TextButton>("Cancel");
        cancelConflictButton->setAccessible(true);
        cancelConflictButton->setWantsKeyboardFocus(true);
        cancelConflictButton->setComponentID("instrumentControl." + juce::String::fromUTF8(control.id.c_str()) + ".cancelConflict");
        cancelConflictButton->setTitle("Cancel conflicting MIDI source for " + juce::String::fromUTF8(control.displayName.c_str()));
        cancelConflictButton->setTooltip("Keep the previous MIDI source");
        cancelConflictButton->setVisible(false);
        cancelConflictButton->onClick = [this, rowIndex] { cancelPendingBinding(rowIndex); };
        cancelConflictButtons.push_back(std::move(cancelConflictButton));
    }
    refreshing = false;
    for (auto& slider : sliders)
        addAndMakeVisible(*slider);
    for (auto& selector : controllerSelectors)
        addAndMakeVisible(*selector);
    for (auto& selector : channelSelectors)
        addAndMakeVisible(*selector);
    for (auto& button : learnButtons)
        addAndMakeVisible(*button);
    for (auto& button : clearButtons)
        addAndMakeVisible(*button);
    for (auto& button : restoreButtons)
        addAndMakeVisible(*button);
    for (auto& button : replaceButtons)
        addAndMakeVisible(*button);
    for (auto& button : cancelConflictButtons)
        addAndMakeVisible(*button);
    for (auto& button : replaceButtons)
        button->setVisible(false);
    for (auto& button : cancelConflictButtons)
        button->setVisible(false);
    addAndMakeVisible(sortSelector);
    addAndMakeVisible(assignmentsDrawerButton);
    addAndMakeVisible(mixerSurfaceButton);
    addAndMakeVisible(instrumentSurfaceButton);
    filterEditor.toFront(false);
    sortSelector.toFront(false);
    assignmentsDrawerButton.toFront(false);
    resized();
    repaint();
}

int InstrumentControlsWorkbenchView::preferredContentHeight(const int width) const noexcept
{
    const auto columns = assignmentsDrawerOpen ? 1 : (width >= 760 ? 3 : width >= 500 ? 2 : 1);
    const auto matchingCount = static_cast<std::size_t>(std::count_if(rows.begin(), rows.end(),
        [this](const auto& row) { return matchesFilter(row); }));
    const auto rowCount = (matchingCount + static_cast<std::size_t>(columns) - 1)
        / static_cast<std::size_t>(columns);
    const auto headerHeight = width < 500 ? 156 : 124;
    return std::max(178, headerHeight + static_cast<int>(rowCount) * 112);
}

void InstrumentControlsWorkbenchView::observeMidiCc(const std::uint8_t channel,
                                                     const std::uint8_t controllerNumber,
                                                     const std::uint8_t value,
                                                     const std::uint64_t nowMs)
{
    const auto matchesChannel = [channel](const auto& binding)
    {
        return binding.channelScope.kind == drs::engine::RuntimeMidiChannelScopeKind::any
            || binding.channelScope.channel == channel;
    };
    for (std::size_t index = 0; index < sourceBindings.size(); ++index)
    {
        const auto& binding = sourceBindings[index];
        if (!binding.enabled || binding.controllerNumber != controllerNumber
            || !matchesChannel(binding))
            continue;
        const auto row = std::find_if(rows.begin(), rows.end(), [&](const auto& candidate)
        {
            return candidate.id == binding.controlId;
        });
        if (row == rows.end())
            continue;
        const auto rowIndex = static_cast<std::size_t>(std::distance(rows.begin(), row));
        if (rowIndex < sliders.size())
        {
            const juce::ScopedValueSetter<bool> guard(refreshing, true);
            sliders[rowIndex]->setValue(static_cast<double>(value) / 127.0,
                                        juce::dontSendNotification);
        }
        const auto status = "Active · Ch " + std::to_string(channel)
            + " CC " + std::to_string(controllerNumber)
            + " = " + std::to_string(value);
        midiSourceStatuses[binding.controlId] = status;
        rows[rowIndex].detail = rows[rowIndex].detail.replace(
            "Awaiting MIDI input", juce::String::fromUTF8(status.c_str()));
    }
    repaint();
    const auto destinationId = learnCoordinator.destinationId();
    const auto learned = learnCoordinator.observeCc(channel, controllerNumber, value, nowMs);
    if (!learned.has_value() || bindingChangedCallback == nullptr)
        return;
    for (std::size_t index = 0; index < rows.size(); ++index)
        if (rows[index].id == destinationId && index < learnButtons.size())
            learnButtons[index]->setTitle("Learned CC " + juce::String(learned->controllerNumber)
                                          + " for " + rows[index].title);
    bindingChangedCallback(destinationId, learned->controllerNumber,
                           learned->channel);
}

std::string InstrumentControlsWorkbenchView::midiSourceStatus(const std::string& controlId) const
{
    const auto found = midiSourceStatuses.find(controlId);
    return found == midiSourceStatuses.end() ? "Unassigned" : found->second;
}

void InstrumentControlsWorkbenchView::armMidiLearn(const std::string& controlId,
                                                    const std::uint64_t nowMs)
{
    learnCoordinator.arm(controlId, nowMs);
    for (std::size_t index = 0; index < rows.size(); ++index)
        if (rows[index].id == controlId && index < learnButtons.size())
        {
            learnButtons[index]->setTitle("Learning MIDI source for " + rows[index].title
                                          + ". Press Escape to cancel");
            learnButtons[index]->grabKeyboardFocus();
        }
    if (learnRequestedCallback != nullptr)
        learnRequestedCallback(controlId);
}

void InstrumentControlsWorkbenchView::cancelMidiLearn() noexcept
{
    const auto destination = learnCoordinator.destinationId();
    learnCoordinator.cancel();
    for (std::size_t index = 0; index < rows.size(); ++index)
        if (rows[index].id == destination && index < learnButtons.size())
            learnButtons[index]->setTitle("Learn MIDI source for " + rows[index].title);
}

void InstrumentControlsWorkbenchView::clearMidiAssignment(const std::string& controlId)
{
    if (bindingClearCallback != nullptr)
        bindingClearCallback(controlId);
}

void InstrumentControlsWorkbenchView::restoreMidiAssignment(const std::string& controlId)
{
    if (bindingRestoreCallback != nullptr)
        bindingRestoreCallback(controlId);
}

bool InstrumentControlsWorkbenchView::bindingSourceConflicts(
    const std::string& controlId,
    const int controller,
    const std::uint8_t channel) const
{
    if (controller < 0)
        return false;
    const auto overlaps = [channel](const auto& scope)
    {
        return channel == 0
            || scope.kind == drs::engine::RuntimeMidiChannelScopeKind::any
            || scope.channel == channel;
    };
    return std::any_of(sourceBindings.begin(), sourceBindings.end(), [&](const auto& binding)
    {
        return binding.enabled && binding.controlId != controlId
            && binding.controllerNumber == controller && overlaps(binding.channelScope);
    });
}

void InstrumentControlsWorkbenchView::requestBindingChange(
    const std::size_t rowIndex,
    const int controller,
    const std::uint8_t channel,
    juce::ComboBox& controllerSelector,
    juce::ComboBox& channelSelector)
{
    if (rowIndex >= rows.size() || bindingChangedCallback == nullptr)
        return;
    if (!bindingSourceConflicts(rows[rowIndex].id, controller, channel))
    {
        pendingBindings[rowIndex] = {};
        replaceButtons[rowIndex]->setVisible(false);
        cancelConflictButtons[rowIndex]->setVisible(false);
        bindingChangedCallback(rows[rowIndex].id, controller, channel);
        return;
    }

    auto previousControllerSelection = 1;
    auto previousChannelSelection = 1;
    const auto existing = std::find_if(sourceBindings.begin(), sourceBindings.end(),
                                       [&](const auto& binding)
                                       {
                                           return binding.enabled
                                               && binding.controlId == rows[rowIndex].id;
                                       });
    if (existing != sourceBindings.end())
    {
        previousControllerSelection = existing->controllerNumber + 2;
        previousChannelSelection = existing->channelScope.kind
            == drs::engine::RuntimeMidiChannelScopeKind::any
            ? 1 : static_cast<int>(existing->channelScope.channel) + 1;
    }
    pendingBindings[rowIndex] = { true, controller, channel,
                                  previousControllerSelection, previousChannelSelection };
    replaceButtons[rowIndex]->setVisible(true);
    cancelConflictButtons[rowIndex]->setVisible(true);
    assignmentsDrawerButton.setTitle("MIDI assignment conflict — choose Replace or Cancel");
    assignmentsDrawerButton.grabKeyboardFocus();
    resized();
    repaint();
}

void InstrumentControlsWorkbenchView::applyPendingBinding(const std::size_t rowIndex)
{
    if (rowIndex >= pendingBindings.size() || !pendingBindings[rowIndex].active
        || bindingChangedCallback == nullptr)
        return;
    const auto pending = pendingBindings[rowIndex];
    pendingBindings[rowIndex] = {};
    replaceButtons[rowIndex]->setVisible(false);
    cancelConflictButtons[rowIndex]->setVisible(false);
    bindingChangedCallback(rows[rowIndex].id, pending.controller, pending.channel);
    resized();
    repaint();
}

void InstrumentControlsWorkbenchView::cancelPendingBinding(const std::size_t rowIndex)
{
    if (rowIndex >= pendingBindings.size() || !pendingBindings[rowIndex].active)
        return;
    const auto pending = pendingBindings[rowIndex];
    pendingBindings[rowIndex] = {};
    if (rowIndex < controllerSelectors.size())
        controllerSelectors[rowIndex]->setSelectedId(pending.previousControllerSelection,
                                                      juce::dontSendNotification);
    if (rowIndex < channelSelectors.size())
        channelSelectors[rowIndex]->setSelectedId(pending.previousChannelSelection,
                                                  juce::dontSendNotification);
    replaceButtons[rowIndex]->setVisible(false);
    cancelConflictButtons[rowIndex]->setVisible(false);
    assignmentsDrawerButton.setTitle(assignmentsDrawerOpen
                                          ? "Return to instrument controls"
                                          : "Open MIDI assignments drawer");
    resized();
    repaint();
}

void InstrumentControlsWorkbenchView::paint(juce::Graphics& graphics)
{
    graphics.fillAll(visual::surfaceSubtle);
    graphics.setColour(visual::text);
    graphics.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    graphics.drawText(assignmentsDrawerOpen ? "MIDI Assignments"
                                           : (activeSurface == Surface::mixer ? "Mixer" : "Instrument Controls"),
                      18, 14, getWidth() - 36, 24, juce::Justification::left);
    graphics.setColour(visual::textMuted);
    graphics.setFont(juce::FontOptions(12.0f));
    graphics.drawText(assignmentsDrawerOpen
                          ? "Full-height source, channel, Learn, and provenance editing"
                          : (activeSurface == Surface::mixer
                                 ? "Master, bus, and kit-piece gain and pan controls"
                                 : "Tuning, envelope, dynamics, and tone parameters"), 18, 39,
                      getWidth() - 36, 18, juce::Justification::left);
    if (rows.empty())
    {
        graphics.drawText("No imported controls yet", 18, 78, getWidth() - 36, 24,
                          juce::Justification::left);
        return;
    }
    const auto columns = assignmentsDrawerOpen ? 1 : (getWidth() >= 760 ? 3 : getWidth() >= 500 ? 2 : 1);
    const auto gap = 10;
    const auto cardWidth = std::max(120, (getWidth() - 36 - gap * (columns - 1)) / columns);
    std::size_t displayIndex = 0;
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        if (!matchesFilter(rows[index]))
            continue;
        const auto column = static_cast<int>(displayIndex % static_cast<std::size_t>(columns));
        const auto row = static_cast<int>(displayIndex / static_cast<std::size_t>(columns));
        const auto contentTop = getWidth() < 500 ? 136 : 104;
        const juce::Rectangle<int> bounds(18 + column * (cardWidth + gap),
                                          contentTop + row * 112, cardWidth, 100);
        graphics.setColour(visual::surfaceRaised);
        graphics.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        graphics.setColour(visual::border);
        graphics.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
        graphics.setColour(visual::text);
        graphics.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        graphics.drawText(rows[index].title, bounds.reduced(10).removeFromTop(22),
                          juce::Justification::left);
        graphics.setColour(visual::textMuted);
        graphics.setFont(juce::FontOptions(11.0f));
        graphics.drawText(rows[index].detail, bounds.reduced(10).removeFromTop(22).translated(0, 20),
                          juce::Justification::left);
        const auto hasFocus = (index < sliders.size() && sliders[index]->hasKeyboardFocus(true))
            || (index < controllerSelectors.size() && controllerSelectors[index]->hasKeyboardFocus(true))
            || (index < channelSelectors.size() && channelSelectors[index]->hasKeyboardFocus(true))
            || (index < learnButtons.size() && learnButtons[index]->hasKeyboardFocus(true))
            || (index < clearButtons.size() && clearButtons[index]->hasKeyboardFocus(true))
            || (index < restoreButtons.size() && restoreButtons[index]->hasKeyboardFocus(true))
            || (index < replaceButtons.size() && replaceButtons[index]->hasKeyboardFocus(true))
            || (index < cancelConflictButtons.size() && cancelConflictButtons[index]->hasKeyboardFocus(true));
        if (hasFocus)
            visual::drawFocusRing(graphics, bounds.reduced(1.0f).toFloat());
        ++displayIndex;
    }
}

void InstrumentControlsWorkbenchView::resized()
{
    const auto buttonWidth = 96;
    const auto sortWidth = 86;
    const auto compact = getWidth() < 500;
    const auto sortX = std::max(110, getWidth() - buttonWidth - sortWidth - 12);
    mixerSurfaceButton.setBounds(18, 58, 62, 28);
    instrumentSurfaceButton.setBounds(84, 58, 102, 28);
    if (compact)
    {
        filterEditor.setBounds(18, 90, std::max(100, getWidth() - sortWidth - 30), 28);
        sortSelector.setBounds(getWidth() - sortWidth - 12, 90, sortWidth, 28);
        assignmentsDrawerButton.setBounds(getWidth() - buttonWidth - 18, 58, buttonWidth, 28);
    }
    else
    {
        filterEditor.setBounds(192, 58, std::max(72, sortX - 202), 28);
        sortSelector.setBounds(sortX, 58, sortWidth, 28);
        assignmentsDrawerButton.setBounds(std::max(sortX + sortWidth + 8, getWidth() - buttonWidth - 18),
                                          58, buttonWidth, 28);
    }
    const auto columns = assignmentsDrawerOpen ? 1 : (getWidth() >= 760 ? 3 : getWidth() >= 500 ? 2 : 1);
    const auto gap = 10;
    const auto cardWidth = std::max(120, (getWidth() - 36 - gap * (columns - 1)) / columns);
    std::size_t displayIndex = 0;
    for (std::size_t index = 0; index < sliders.size(); ++index)
    {
        const auto visible = matchesFilter(rows[index])
            && (activeSurface == Surface::mixer
                    ? rows[index].category == drs::engine::RuntimeInstrumentControlCategory::mixer
                    : rows[index].category != drs::engine::RuntimeInstrumentControlCategory::mixer);
        const auto conflictPending = index < pendingBindings.size() && pendingBindings[index].active;
        sliders[index]->setVisible(visible && !assignmentsDrawerOpen);
        controllerSelectors[index]->setVisible(visible);
        channelSelectors[index]->setVisible(visible);
        learnButtons[index]->setVisible(visible && !conflictPending);
        clearButtons[index]->setVisible(visible);
        restoreButtons[index]->setVisible(visible);
        replaceButtons[index]->setVisible(visible && conflictPending);
        cancelConflictButtons[index]->setVisible(visible && conflictPending);
        if (!visible)
            continue;
        const auto column = static_cast<int>(displayIndex % static_cast<std::size_t>(columns));
        const auto row = static_cast<int>(displayIndex / static_cast<std::size_t>(columns));
        const auto x = 18 + column * (cardWidth + gap) + 10;
        const auto y = (compact ? 136 : 104) + row * 112;
        const auto width = cardWidth - 20;
        sliders[index]->setBounds(x, y + 36, width, 20);
        const auto controllerWidth = std::min(92, std::max(70, width / 3));
        const auto channelWidth = std::min(70, std::max(54, width / 4));
        controllerSelectors[index]->setBounds(x, y + (assignmentsDrawerOpen ? 38 : 70), controllerWidth, 22);
        const auto buttonX = x + controllerWidth + channelWidth + 12;
        const auto assignmentY = assignmentsDrawerOpen ? y + 38 : y + 70;
        channelSelectors[index]->setBounds(x + controllerWidth + 4, assignmentY,
                                           channelWidth, 22);
        const auto actionWidth = std::max(42, width - (buttonX - x) - 72);
        if (conflictPending)
        {
            const auto resolutionWidth = std::max(42, (actionWidth - 4) / 2);
            replaceButtons[index]->setBounds(buttonX, assignmentY, resolutionWidth, 28);
            cancelConflictButtons[index]->setBounds(buttonX + resolutionWidth + 4, assignmentY,
                                                    actionWidth - resolutionWidth - 4, 28);
        }
        else
        {
            learnButtons[index]->setBounds(buttonX, assignmentY, actionWidth, 28);
        }
        clearButtons[index]->setBounds(x + width - 32, assignmentY, 32, 28);
        restoreButtons[index]->setBounds(x + width - 68, assignmentY, 32, 28);
        ++displayIndex;
    }
}

bool InstrumentControlsWorkbenchView::matchesFilter(const Row& row) const
{
    const auto filter = filterEditor.getText().trim().toLowerCase();
    const auto surfaceMatches = assignmentsDrawerOpen
        || (activeSurface == Surface::mixer
                ? row.category == drs::engine::RuntimeInstrumentControlCategory::mixer
                : row.category != drs::engine::RuntimeInstrumentControlCategory::mixer);
    return surfaceMatches && (filter.isEmpty()
        || row.title.toLowerCase().contains(filter)
        || row.detail.toLowerCase().contains(filter));
}

bool InstrumentControlsWorkbenchView::keyPressed(const juce::KeyPress& key,
                                                  juce::Component*)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey
        && learnCoordinator.isArmed())
    {
        cancelMidiLearn();
        return true;
    }
    return false;
}
} // namespace drs::app::authoring
