#include "drs/engine/AuthoringSession.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "shared/PerformancePanel.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "standalone/MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
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
    for (auto* child : root.getChildren())
        if (auto* match = findDescendantById(*child, id); match != nullptr)
            return match;
    return nullptr;
}

juce::Component* findDescendantByIdPrefix(juce::Component& root, const juce::String& prefix)
{
    if (root.getComponentID().startsWith(prefix))
        return &root;
    for (auto* child : root.getChildren())
        if (auto* match = findDescendantByIdPrefix(*child, prefix); match != nullptr)
            return match;
    return nullptr;
}

template <typename ComponentType>
ComponentType& requireComponent(juce::Component& root,
                                const juce::String& id,
                                const std::string& shellName)
{
    auto* component = dynamic_cast<ComponentType*>(findDescendantById(root, id));
    require(component != nullptr,
            shellName + " is missing Phase 9 component " + id.toStdString() + ".");
    return *component;
}

void requireInside(juce::Component& root,
                   juce::Component& child,
                   const std::string& context)
{
    const auto bounds = root.getLocalArea(&child, child.getLocalBounds());
    require(!bounds.isEmpty() && root.getLocalBounds().contains(bounds),
            context + " escaped the supported shell bounds.");
}

void requireAccessible(juce::Component& component, const std::string& context)
{
    require(component.getTitle().isNotEmpty()
                && component.getDescription().isNotEmpty()
                && component.getHelpText().isNotEmpty()
                && component.getExplicitFocusOrder() > 0,
            context + " must retain a title, description, help text, and focus order.");
}

drs::engine::RuntimeProjectModel loadPhase9FixtureProject()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 9 requires the Phase 2 reference project.");
    auto project = loaded.project;
    require(!project.authoring.fxSlots.empty(),
            "Phase 9 requires an authored FX slot for the Routing journey.");

    auto& selectedFx = project.authoring.fxSlots.front();
    selectedFx.effectType = "drs.gain";
    selectedFx.effectVersion = 1;
    selectedFx.unavailable = false;
    selectedFx.legacyInert = false;
    selectedFx.parameters.clear();
    const auto* descriptor = drs::engine::findCuratedDspEffect(
        selectedFx.effectType, selectedFx.effectVersion);
    require(descriptor != nullptr && !descriptor->parameters.empty(),
            "Phase 9 requires the curated Gain descriptor.");
    for (const auto& parameter : descriptor->parameters)
        selectedFx.parameters.push_back({ std::string(parameter.id), parameter.defaultValue });

    drs::engine::RuntimeProjectRoutingBusDefinition bus;
    bus.id = "phase9-master";
    bus.displayName = "Instrument Master";
    bus.inputSourceId = "master";
    for (std::size_t index = 0; index < std::min<std::size_t>(2, project.authoring.fxSlots.size()); ++index)
        bus.fxSlotIds.push_back(project.authoring.fxSlots[index].id);
    project.authoring.routingBuses = { std::move(bus) };
    return project;
}

void prepareDefaultPerformance(drs::plugin::Processor& processor,
                               const std::string& shellName)
{
    processor.prepareToPlay(44100.0, 512);
    processor.getEngineFacade().resetSessionStateToDefault();
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
            shellName + " did not finish preparing the default performance.");
    processor.serviceMessageThreadWork();
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
    require(processor.getEngineFacade().getPerformanceActivationPayload() != nullptr,
            shellName + " did not install the default performance activation.");
}

float auditionThreeNotes(drs::plugin::Processor& processor)
{
    float maximumMagnitude = 0.0f;
    for (const auto midiNote : { 57, 69, 57 })
    {
        juce::MidiBuffer noteOn;
        noteOn.addEvent(juce::MidiMessage::noteOn(1, midiNote,
                                                  static_cast<juce::uint8>(104)), 0);
        for (int blockIndex = 0; blockIndex < 4; ++blockIndex)
        {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            processor.processBlock(buffer, noteOn);
            noteOn.clear();
            maximumMagnitude = std::max(maximumMagnitude,
                                        buffer.getMagnitude(0, buffer.getNumSamples()));
        }
        juce::AudioBuffer<float> releaseBuffer(2, 512);
        juce::MidiBuffer noteOff;
        noteOff.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0);
        releaseBuffer.clear();
        processor.processBlock(releaseBuffer, noteOff);
    }
    return maximumMagnitude;
}

struct PerformanceControlState
{
    std::string authoredId;
    double value = 0.0;
};

PerformanceControlState adjustFirstPerformanceControl(drs::app::PerformancePanel& panel,
                                                       drs::plugin::Processor& processor,
                                                       const std::string& shellName)
{
    panel.refreshNow();
    auto& disclosure = requireComponent<juce::Button>(
        panel, "performanceMacroStripToggleButton", shellName);
    if (disclosure.getButtonText() == "Show Controls")
        disclosure.onClick();

    auto* widget = findDescendantByIdPrefix(panel, "performanceMixerWidget.");
    require(widget != nullptr && widget->isEnabled(),
            shellName + " Perform surface did not expose an adjustable published control.");
    const auto authoredId = widget->getComponentID()
                                .fromFirstOccurrenceOf("performanceMixerWidget.", false, false)
                                .toStdString();

    if (auto* slider = dynamic_cast<juce::Slider*>(widget))
    {
        const auto span = slider->getMaximum() - slider->getMinimum();
        const auto nextValue = slider->getValue() < slider->getMinimum() + span * 0.6
            ? slider->getMinimum() + span * 0.72
            : slider->getMinimum() + span * 0.28;
        slider->setValue(nextValue, juce::sendNotificationSync);
    }
    else if (auto* toggle = dynamic_cast<juce::ToggleButton*>(widget))
    {
        toggle->triggerClick();
    }
    else
    {
        require(false, shellName + " published control used an unsupported widget type.");
    }

    const auto descriptors = processor.getEngineFacade().getMacroDescriptors();
    const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(),
                                         [&](const auto& candidate)
                                         {
                                             return candidate.authoredId == authoredId;
                                         });
    require(descriptor != descriptors.end() && std::isfinite(descriptor->currentValue),
            shellName + " published control did not update the shared performance state.");
    return { authoredId, descriptor->currentValue };
}

void requirePerformanceControlPreserved(const PerformanceControlState& expected,
                                        drs::plugin::Processor& processor,
                                        const std::string& shellName)
{
    const auto descriptors = processor.getEngineFacade().getMacroDescriptors();
    const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(),
                                         [&](const auto& candidate)
                                         {
                                             return candidate.authoredId == expected.authoredId;
                                         });
    require(descriptor != descriptors.end()
                && std::abs(descriptor->currentValue - expected.value) < 0.000001,
            shellName + " lost the Perform control value while visiting authoring workspaces.");
}

void qualifySharedVisualAndAccessibility(drs::app::PerformancePanel& performance,
                                         drs::app::AuthoringPanel& authoring,
                                         const std::string& shellName)
{
    using namespace drs::app::authoring;
    require(visual::borderWidth == 1.0f && visual::controlRadius == 2.0f
                && visual::panelRadius == 3.0f && visual::focusWidth == 2.0f,
            "Pass 03 structure and geometry tokens changed after convergence.");
    require(performance.findColour(juce::ScrollBar::backgroundColourId)
                == visual::surfaceSubtle
                && performance.findColour(juce::ScrollBar::thumbColourId)
                    == visual::borderStrong
                && performance.findColour(juce::TooltipWindow::backgroundColourId)
                    == visual::surfaceRaised
                && performance.findColour(juce::TooltipWindow::outlineColourId)
                    == visual::borderStrong,
            shellName + " Perform surface diverged from shared scrollbar/tooltip tokens.");
    require(authoring.findColour(juce::PopupMenu::backgroundColourId)
                == visual::surfaceRaised
                && authoring.findColour(juce::PopupMenu::highlightedBackgroundColourId)
                    == visual::selection
                && authoring.findColour(juce::ScrollBar::thumbColourId)
                    == visual::borderStrong
                && authoring.findColour(juce::TooltipWindow::outlineColourId)
                    == visual::borderStrong,
            shellName + " authoring surface diverged from shared menu/scrollbar/tooltip tokens.");
    require(requireComponent<juce::Button>(authoring, "authoringMacroDeleteButton", shellName)
                .findColour(juce::TextButton::textColourOffId) == visual::error
                && requireComponent<juce::Button>(authoring, "authoringFxDeleteButton", shellName)
                    .findColour(juce::TextButton::textColourOffId) == visual::error,
            shellName + " Macro/Routing destructive actions must share the error role.");

    for (const auto* id : { "performanceDetailsToggleButton",
                            "performanceMacroStripToggleButton" })
        requireAccessible(requireComponent<juce::Component>(performance, id, shellName),
                          shellName + " " + id);
    for (const auto* id : { "authoringWorkbenchMacrosTab",
                            "authoringMacroDefaultSlider",
                            "authoringMacroAssignmentAddButton",
                            "authoringWorkbenchRoutingTab",
                            "authoringFxBypassedToggle",
                            "authoringFxParameterSlider" })
        requireAccessible(requireComponent<juce::Component>(authoring, id, shellName),
                          shellName + " " + id);
}

void qualifyShellSizeAndScaleMatrix(juce::Component& shell,
                                    juce::TabbedComponent& tabs,
                                    drs::app::PerformancePanel& performance,
                                    drs::app::AuthoringPanel& authoring,
                                    const std::vector<juce::Point<int>>& sizes,
                                    const std::string& shellName)
{
    for (const auto size : sizes)
    {
        shell.setSize(size.x, size.y);
        shell.resized();
        tabs.setCurrentTabIndex(0);
        performance.refreshNow();
        requireInside(shell, performance, shellName + " Perform");
        requireInside(shell,
                      requireComponent<juce::Component>(performance, "performanceKeyboard", shellName),
                      shellName + " keyboard");

        tabs.setCurrentTabIndex(1);
        authoring.resized();
        requireInside(shell, authoring, shellName + " Map workspace");
        auto& map = requireComponent<juce::Component>(authoring, "authoringZoneMap", shellName);
        require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
                shellName + " clipped the protected map at " + std::to_string(size.x)
                    + "x" + std::to_string(size.y) + ".");
    }

    shell.setSize(sizes.front().x, sizes.front().y);
    shell.resized();
    for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
    {
        juce::Image image(juce::Image::ARGB,
                          juce::roundToInt(shell.getWidth() * scale),
                          juce::roundToInt(shell.getHeight() * scale), true);
        juce::Graphics graphics(image);
        graphics.addTransform(juce::AffineTransform::scale(scale));
        shell.paintEntireComponent(graphics, true);
        require(image.getWidth() == juce::roundToInt(shell.getWidth() * scale)
                    && image.getHeight() == juce::roundToInt(shell.getHeight() * scale),
                shellName + " failed scaled paint traversal at "
                    + std::to_string(scale) + "x.");
    }
}

void qualifyCrossWorkspaceJourney(juce::Component& shell,
                                  drs::plugin::Processor& processor,
                                  const std::vector<juce::Point<int>>& sizes,
                                  const std::string& shellName)
{
    auto& tabs = requireComponent<juce::TabbedComponent>(shell, "workspaceTabs", shellName);
    require(tabs.getNumTabs() == 2,
            shellName + " must expose Perform and Map for the Phase 9 demonstration.");
    auto* performance = dynamic_cast<drs::app::PerformancePanel*>(tabs.getTabContentComponent(0));
    auto* authoring = dynamic_cast<drs::app::AuthoringPanel*>(tabs.getTabContentComponent(1));
    require(performance != nullptr && authoring != nullptr,
            shellName + " workspace tabs did not retain their shared surface components.");

    require(processor.replaceAuthoringProject(loadPhase9FixtureProject()),
            shellName + " could not load the deterministic Pass 03 demonstration project.");
    authoring->reloadFromSession();
    prepareDefaultPerformance(processor, shellName);
    qualifyShellSizeAndScaleMatrix(shell, tabs, *performance, *authoring, sizes, shellName);
    qualifySharedVisualAndAccessibility(*performance, *authoring, shellName);

    shell.setSize(sizes.front().x, sizes.front().y);
    shell.resized();
    tabs.setCurrentTabIndex(1);
    auto& session = processor.getAuthoringSession();
    require(!session.getProject().authoring.zones.empty(),
            shellName + " needs a playable authoring fixture for convergence.");
    const auto selectedZoneId = session.getProject().authoring.zones.back().id;
    require(session.selectZone(selectedZoneId).applied,
            shellName + " could not select the demonstration zone.");
    authoring->refreshNow();
    auto& map = requireComponent<drs::app::authoring::ZoneMapCanvas>(
        *authoring, "authoringZoneMap", shellName);
    require(map.getSelectionState().primaryZoneId == selectedZoneId,
            shellName + " Map did not reflect the demonstration selection.");

    tabs.setCurrentTabIndex(0);
    const auto performanceState = adjustFirstPerformanceControl(*performance, processor, shellName);
    require(auditionThreeNotes(processor) > 0.0001f,
            shellName + " Perform journey did not produce audible output for three note gestures.");

    tabs.setCurrentTabIndex(1);
    auto& workbenchToggle = requireComponent<juce::Button>(
        *authoring, "authoringWorkbenchToggleButton", shellName);
    if (workbenchToggle.getButtonText() == "Show Workbench")
        workbenchToggle.onClick();
    requireComponent<juce::Button>(*authoring, "authoringWorkbenchMacrosTab", shellName).onClick();
    auto& workbench = requireComponent<juce::Component>(*authoring, "authoringWorkbench", shellName);
    const auto macroWorkbenchHeight = workbench.getHeight();
    auto& macroSlider = requireComponent<juce::Slider>(
        *authoring, "authoringMacroDefaultSlider", shellName);
    require(macroSlider.isEnabled(), shellName + " Macro range must be editable.");
    const auto macroUndoDepth = session.getDocumentState().undoDepth;
    const auto macroSpan = macroSlider.getMaximum() - macroSlider.getMinimum();
    const auto macroValue = macroSlider.getValue() < macroSlider.getMinimum() + macroSpan * 0.55
        ? macroSlider.getMinimum() + macroSpan * 0.70
        : macroSlider.getMinimum() + macroSpan * 0.30;
    macroSlider.setValue(macroValue, juce::dontSendNotification);
    macroSlider.onDragEnd();
    require(session.getDocumentState().undoDepth == macroUndoDepth + 1,
            shellName + " Macro range edit bypassed the document transaction.");
    require(session.undo().applied, shellName + " Macro edit was not undoable.");
    authoring->reloadFromSession();
    require(requireComponent<juce::Button>(
                *authoring, "authoringWorkbenchMacrosTab", shellName).getToggleState()
                && workbench.getHeight() == macroWorkbenchHeight,
            shellName + " lost Macro tab or workbench height across undo.");
    require(session.redo().applied, shellName + " Macro edit was not redoable.");
    authoring->reloadFromSession();

    requireComponent<juce::Button>(*authoring, "authoringWorkbenchRoutingTab", shellName).onClick();
    auto& routingScope = requireComponent<juce::ComboBox>(
        *authoring, "authoringDspScopeSelector", shellName);
    routingScope.setSelectedId(3, juce::sendNotificationSync);
    auto& fxSelector = requireComponent<juce::ComboBox>(
        *authoring, "authoringFxSelector", shellName);
    require(fxSelector.getNumItems() > 0,
            shellName + " Routing path did not expose an insert.");
    fxSelector.setSelectedItemIndex(0, juce::sendNotificationSync);
    const auto routingWorkbenchHeight = workbench.getHeight();
    auto& bypass = requireComponent<juce::ToggleButton>(
        *authoring, "authoringFxBypassedToggle", shellName);
    require(bypass.isEnabled(), shellName + " Routing bypass must be reachable (scope="
                + std::to_string(routingScope.getSelectedId()) + ", buses="
                + std::to_string(session.getProject().authoring.routingBuses.size()) + ", slots="
                + std::to_string(session.getProject().authoring.fxSlots.size()) + ", selectorItems="
                + std::to_string(fxSelector.getNumItems()) + ").");
    const auto routingUndoDepth = session.getDocumentState().undoDepth;
    bypass.setToggleState(!bypass.getToggleState(), juce::dontSendNotification);
    bypass.onClick();
    require(session.getDocumentState().undoDepth == routingUndoDepth + 1,
            shellName + " Routing edit bypassed the document transaction.");
    require(session.undo().applied, shellName + " Routing edit was not undoable.");
    authoring->reloadFromSession();
    require(requireComponent<juce::Button>(
                *authoring, "authoringWorkbenchRoutingTab", shellName).getToggleState()
                && workbench.getHeight() == routingWorkbenchHeight,
            shellName + " lost Routing scope or workbench height across undo.");
    require(session.redo().applied, shellName + " Routing edit was not redoable.");
    authoring->reloadFromSession();

    workbenchToggle.onClick();
    require(workbenchToggle.getButtonText() == "Show Workbench",
            shellName + " did not collapse the workbench at the end of the authoring path.");
    requireComponent<juce::Button>(*authoring, "authoringZoneMapFitAll", shellName).onClick();
    require(session.getSelectedZone().has_value()
                && session.getSelectedZone()->id == selectedZoneId
                && map.getSelectionState().primaryZoneId == selectedZoneId,
            shellName + " lost Map selection after Perform, Macros, and Routing.");

    tabs.setCurrentTabIndex(0);
    performance->refreshNow();
    requirePerformanceControlPreserved(performanceState, processor, shellName);
    tabs.setCurrentTabIndex(1);
    require(session.getSelectedZone()->id == selectedZoneId,
            shellName + " lost authoring selection on the final Map return.");

    for (const auto* id : { "authoringWorkbenchWaveformTab",
                            "authoringWorkbenchGroupsTab",
                            "authoringWorkbenchPerformanceTab",
                            "authoringWorkbenchArticulationsTab" })
    {
        if (workbenchToggle.getButtonText() == "Show Workbench")
            workbenchToggle.onClick();
        auto& tab = requireComponent<juce::Button>(*authoring, id, shellName);
        tab.onClick();
        require(tab.getToggleState()
                    && map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
                shellName + " shared control changes regressed " + id + ".");
    }
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        auto standalone = std::make_unique<drs::standalone::MainComponent>(false);
        qualifyCrossWorkspaceJourney(*standalone, standalone->getProcessor(),
                                     { { 1120, 800 }, { 900, 700 } }, "Standalone");

        auto pluginProcessor = std::make_unique<drs::plugin::Processor>();
        std::unique_ptr<juce::AudioProcessorEditor> pluginEditor(pluginProcessor->createEditor());
        require(pluginEditor != nullptr, "VST3 editor could not be created for Phase 9.");
        qualifyCrossWorkspaceJourney(*pluginEditor, *pluginProcessor,
                                     { { 900, 700 }, { 820, 700 }, { 760, 620 } }, "VST3");

        std::cout << "Open Workbench Phase 9 Pass 03 convergence qualification passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 9 Pass 03 convergence qualification failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
