#include "shared/authoring/InstrumentControlsWorkbenchView.h"
#include "shared/authoring/MidiLearnCoordinator.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <functional>
#include <utility>

using namespace drs::engine;
using namespace drs::app::authoring;

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    InstrumentControlsWorkbenchView view;
    RuntimeProjectInstrumentControlDefinition control;
    control.id = "sfz.cc.20";
    control.displayName = "Kick In";
    control.category = RuntimeInstrumentControlCategory::mixer;
    control.unit = RuntimeInstrumentControlUnit::decibels;
    control.kind = RuntimeInstrumentControlKind::decibels;
    control.displayMinimum = -96.0;
    control.displayMaximum = 6.0;
    control.displayPrecision = 1;
    control.visible = true;
    control.displayOrder = 0;
    control.importedSourceController = 20;
    RuntimeProjectMidiControlBindingDefinition binding;
    binding.id = "binding.sfz.cc.20";
    binding.controlId = control.id;
    binding.controllerNumber = 20;
    auto decay = control;
    decay.id = "envelope.decay";
    decay.displayName = "Decay";
    decay.category = RuntimeInstrumentControlCategory::envelope;
    decay.unit = RuntimeInstrumentControlUnit::seconds;
    decay.kind = RuntimeInstrumentControlKind::seconds;
    decay.displayMinimum = 0.01;
    decay.displayMaximum = 4.0;
    decay.displayPrecision = 2;
    decay.displayOrder = 1;
    decay.importedSourceController = 41;
    auto decayBinding = binding;
    decayBinding.id = "binding.envelope.decay.cc41";
    decayBinding.controlId = decay.id;
    decayBinding.controllerNumber = 41;
    bool bindingChanged = false;
    bool decayBindingApplied = false;
    bool bindingCleared = false;
    bool bindingRestored = false;
    bool learnRequested = false;
    view.setBindingChangedCallback([&](const std::string& id, const int controller, const std::uint8_t channel)
    {
        bindingChanged = id == control.id && controller == 21 && channel == 2;
        decayBindingApplied = id == decay.id && controller == 20 && channel == 0;
    });
    view.setBindingClearCallback([&](const std::string& id)
    {
        bindingCleared = id == control.id;
    });
    view.setBindingRestoreCallback([&](const std::string& id)
    {
        bindingRestored = id == control.id;
    });
    view.setLearnRequestedCallback([&](const std::string& id)
    {
        learnRequested = id == control.id;
    });
    view.setControls({ control, decay }, { binding, decayBinding });
    assert(view.midiSourceStatus(control.id) == "Awaiting MIDI input");
    assert(view.preferredContentHeight(820) >= 120);
    view.setBounds(0, 0, 820, view.preferredContentHeight(820));
    const auto wideHeight = view.preferredContentHeight(1120);
    const auto compactHeight = view.preferredContentHeight(320);
    assert(wideHeight >= 120 && compactHeight >= wideHeight);
    view.setBounds(0, 0, 1120, wideHeight);
    view.setBounds(0, 0, 320, compactHeight);
    for (const auto size : { std::pair { 820, 700 }, std::pair { 1120, 800 }, std::pair { 320, compactHeight } })
    {
        view.setBounds(0, 0, size.first, size.second);
        juce::Image image(juce::Image::ARGB, size.first, size.second, true);
        juce::Graphics graphics(image);
        view.paintEntireComponent(graphics, true);
        assert(visual::surfaceSubtle.getPerceivedBrightness() > 0.5f);
        juce::MemoryOutputStream pngBytes;
        assert(juce::PNGImageFormat().writeImageToStream(image, pngBytes)
                   && pngBytes.getDataSize() > 0);
    }
    assert(view.getComponentID() == "authoringInstrumentControlsContent");
    assert(view.findChildWithID("instrumentControlsFilter") != nullptr);
    assert(view.findChildWithID("instrumentControlsSort") != nullptr);
    auto* mixerSurface = view.findChildWithID("instrumentControlsMixerSurface");
    auto* parameterSurface = view.findChildWithID("instrumentControlsParameterSurface");
    assert(mixerSurface != nullptr && parameterSurface != nullptr);
    assert(mixerSurface->getTitle().contains("mixer")
               && parameterSurface->getTitle().contains("parameter"));
    auto* valueEditor = view.findChildWithID("instrumentControl.sfz.cc.20.value");
    auto* controllerEditor = view.findChildWithID("instrumentControl.sfz.cc.20.controller");
    auto* channelEditor = view.findChildWithID("instrumentControl.sfz.cc.20.channel");
    assert(valueEditor != nullptr && valueEditor->getTitle() == "Kick In");
    assert(controllerEditor != nullptr && controllerEditor->getTitle().contains("MIDI CC source"));
    assert(channelEditor != nullptr && channelEditor->getTitle().contains("MIDI channel scope"));
    auto* assignmentsDrawer = view.findChildWithID("instrumentControlsAssignmentsDrawerToggle");
    assert(assignmentsDrawer != nullptr && !view.isAssignmentsDrawerOpen());
    assert(assignmentsDrawer->getTitle().contains("MIDI assignments"));
    std::function<void(juce::Component&)> assertAccessibleControls =
        [&](juce::Component& component)
    {
        const auto interactive = dynamic_cast<juce::Slider*>(&component) != nullptr
            || dynamic_cast<juce::ComboBox*>(&component) != nullptr
            || dynamic_cast<juce::Button*>(&component) != nullptr
            || dynamic_cast<juce::TextEditor*>(&component) != nullptr;
        if (interactive)
        {
            assert(!component.getComponentID().isEmpty() && !component.getTitle().isEmpty());
            assert(component.getWantsKeyboardFocus());
            auto handler = component.createAccessibilityHandler();
            assert(handler != nullptr && !handler->isIgnored() && !handler->getTitle().isEmpty());
            if (dynamic_cast<juce::Slider*>(&component) != nullptr)
                assert(handler->getValueInterface() != nullptr);
        }
        for (int child = 0; child < component.getNumChildComponents(); ++child)
            assertAccessibleControls(*component.getChildComponent(child));
    };
    assertAccessibleControls(view);
    view.setAssignmentsDrawerOpen(true);
    assert(view.isAssignmentsDrawerOpen() && view.preferredContentHeight(320) >= 120);
    view.setBounds(0, 0, 320, view.preferredContentHeight(320));
    for (int child = 0; child < view.getNumChildComponents(); ++child)
        if (auto* component = view.getChildComponent(child); component->isVisible())
            assert(component->getBoundsInParent().getWidth() >= 32
                       && component->getBoundsInParent().getRight() <= view.getWidth());
    view.setAssignmentsDrawerOpen(false);
    assert(!view.isAssignmentsDrawerOpen());
    if (valueEditor != nullptr)
    {
        view.observeMidiCc(2, 20, 64, 999);
        assert(std::abs(dynamic_cast<juce::Slider*>(valueEditor)->getValue()
                            - (64.0 / 127.0)) < 0.001);
        assert(view.midiSourceStatus(control.id) == "Active · Ch 2 CC 20 = 64");
    }
    const auto expandedHeight = view.preferredContentHeight(400);
    if (auto* filter = dynamic_cast<juce::TextEditor*>(view.findChildWithID("instrumentControlsFilter")))
        filter->setText("Kick", true);
    assert(view.preferredContentHeight(400) < expandedHeight);
    if (auto* filter = dynamic_cast<juce::TextEditor*>(view.findChildWithID("instrumentControlsFilter")))
        filter->clear();
    if (auto* combo = dynamic_cast<juce::ComboBox*>(controllerEditor))
        combo->setSelectedId(23, juce::sendNotificationSync);
    if (auto* combo = dynamic_cast<juce::ComboBox*>(channelEditor))
        combo->setSelectedId(3, juce::sendNotificationSync);
    bindingChanged = false;
    view.armMidiLearn(control.id, 1000);
    assert(view.findChildWithID("instrumentControl.sfz.cc.20.learn")->getTitle()
               .contains("Press Escape"));
    view.cancelMidiLearn();
    assert(view.findChildWithID("instrumentControl.sfz.cc.20.learn")->getTitle()
               == "Learn MIDI source for Kick In");
    view.armMidiLearn(control.id, 1000);
    view.observeMidiCc(2, 21, 80, 1001);
    assert(bindingChanged);
    assert(view.findChildWithID("instrumentControl.sfz.cc.20.learn")->getTitle()
               .contains("Learned CC 21"));
    view.clearMidiAssignment(control.id);
    view.restoreMidiAssignment(control.id);
    assert(bindingChanged && bindingCleared && bindingRestored && learnRequested);
    auto* decayController = view.findChildWithID("instrumentControl.envelope.decay.controller");
    auto* decayReplace = view.findChildWithID("instrumentControl.envelope.decay.replace");
    auto* decayCancel = view.findChildWithID("instrumentControl.envelope.decay.cancelConflict");
    assert(decayController != nullptr && decayReplace != nullptr && decayCancel != nullptr);
    if (auto* combo = dynamic_cast<juce::ComboBox*>(decayController))
    {
        combo->setSelectedId(22, juce::sendNotificationSync); // CC20 conflicts with Kick In.
        assert(decayReplace->isVisible() && decayCancel->isVisible());
        dynamic_cast<juce::Button*>(decayCancel)->onClick();
        assert(!decayReplace->isVisible() && combo->getSelectedId() == 43); // CC41 restored.
        combo->setSelectedId(22, juce::sendNotificationSync);
        dynamic_cast<juce::Button*>(decayReplace)->onClick();
        assert(decayBindingApplied);
    }
    view.setSurface(InstrumentControlsWorkbenchView::Surface::mixer);
    assert(view.surface() == InstrumentControlsWorkbenchView::Surface::mixer);
    assert(view.preferredContentHeight(400) >= 120);
    view.setSurface(InstrumentControlsWorkbenchView::Surface::instrumentControls);
    assert(view.surface() == InstrumentControlsWorkbenchView::Surface::instrumentControls);
    view.setControls({}, {});
    assert(view.preferredContentHeight(320) >= 178
               && view.findChildWithID("instrumentControlsFilter") != nullptr);
    std::vector<RuntimeProjectInstrumentControlDefinition> largeControls;
    largeControls.reserve(128);
    for (int index = 0; index < 128; ++index)
    {
        auto largeControl = control;
        largeControl.id = "large." + std::to_string(index);
        largeControl.displayName = "Large Control " + std::to_string(index);
        largeControl.displayOrder = index;
        largeControls.push_back(std::move(largeControl));
    }
    view.setSurface(InstrumentControlsWorkbenchView::Surface::mixer);
    view.setControls(std::move(largeControls), {});
    assert(view.preferredContentHeight(320) > 1000);
    view.setControls({ control, decay }, { binding, decayBinding });
    view.setSurface(InstrumentControlsWorkbenchView::Surface::instrumentControls);
    MidiLearnCoordinator learn;
    learn.arm(control.id, 1000);
    assert(!learn.observeCc(1, 64, 127, 1001).has_value());
    assert(!learn.observeCc(1, 120, 0, 1002).has_value());
    assert(!learn.observeCc(1, 123, 0, 1003).has_value());
    const auto learned = learn.observeCc(2, 21, 80, 1004);
    assert(learned.has_value() && learned->controllerNumber == 21 && learned->channel == 2);
    learn.arm(control.id, 1000, 10);
    assert(!learn.observeCc(1, 22, 80, 1011).has_value() && !learn.isArmed());
    std::cout << "instrument control UI passed\n";
    return 0;
}
