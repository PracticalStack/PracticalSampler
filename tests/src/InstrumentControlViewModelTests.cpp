#include "shared/authoring/InstrumentControlViewModels.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cassert>
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    using namespace drs::engine;
    using namespace drs::app::authoring;

    RuntimeProjectInstrumentControlDefinition mixer;
    mixer.id = "mixer.kick";
    mixer.displayName = "Kick In";
    mixer.category = RuntimeInstrumentControlCategory::mixer;
    mixer.unit = RuntimeInstrumentControlUnit::decibels;
    mixer.displayMinimum = -96.0;
    mixer.displayMaximum = 6.0;
    mixer.displayPrecision = 1;
    mixer.normalizedDefault = 0.5;
    mixer.displayOrder = 2;
    RuntimeProjectInstrumentControlDefinition decay = mixer;
    decay.id = "envelope.decay";
    decay.displayName = "Decay";
    decay.category = RuntimeInstrumentControlCategory::envelope;
    decay.unit = RuntimeInstrumentControlUnit::seconds;
    decay.displayMinimum = 0.0;
    decay.displayMaximum = 4.0;
    decay.displayPrecision = 2;
    decay.displayOrder = 1;

    RuntimeProjectMidiControlBindingDefinition kickBinding;
    kickBinding.id = "binding.kick";
    kickBinding.controlId = mixer.id;
    kickBinding.controllerNumber = 20;
    kickBinding.imported = true;
    RuntimeProjectMidiControlBindingDefinition decayBinding = kickBinding;
    decayBinding.id = "binding.decay";
    decayBinding.controlId = decay.id;
    decayBinding.controllerNumber = 21;

    const auto rows = buildInstrumentControlViewRows(
        { mixer, decay }, { kickBinding, decayBinding }, {{ mixer.id, 1.0 }, { decay.id, 1.0 }});
    assert(rows.size() == 2);
    assert(rows.front().id == decay.id && rows.front().valueText.find("4.00 s") != std::string::npos);
    assert(rows.back().mixerSurface && rows.back().valueText.find("6.0 dB") != std::string::npos);
    assert(rows.back().accessibleText.find("CC 20") != std::string::npos);

    auto conflict = decayBinding;
    conflict.id = "binding.conflict";
    conflict.controlId = decay.id;
    conflict.controllerNumber = 20;
    const auto assignments = buildInstrumentControlAssignmentViewRows(
        { mixer, decay }, { kickBinding, conflict });
    assert(assignments.size() == 2);
    assert(assignments.back().conflict);
    assert(assignments.back().statusText.find("Replace or Cancel") != std::string::npos);

    std::cout << "instrument control view models passed\n";
    return 0;
}
