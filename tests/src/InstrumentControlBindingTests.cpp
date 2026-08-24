#include "drs/engine/InstrumentControlBinding.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        RuntimeProjectInstrumentControlDefinition kick;
        kick.id = "mixer.kick";
        kick.displayName = "Kick";
        kick.normalizedDefault = 0.25;
        RuntimeProjectInstrumentControlDefinition snare = kick;
        snare.id = "mixer.snare";
        snare.normalizedDefault = 0.75;

        RuntimeProjectMidiControlBindingDefinition anyKick;
        anyKick.id = "binding.kick.cc20";
        anyKick.controlId = kick.id;
        anyKick.controllerNumber = 20;

        RuntimeProjectMidiControlBindingDefinition channelSnare;
        channelSnare.id = "binding.snare.cc21.ch2";
        channelSnare.controlId = snare.id;
        channelSnare.controllerNumber = 21;
        channelSnare.channelScope.kind = RuntimeMidiChannelScopeKind::exact;
        channelSnare.channelScope.channel = 2;

        InstrumentControlBindingTable table;
        std::vector<RuntimeInstrumentControlBindingIssue> issues;
        require(table.compile({ kick, snare }, { anyKick, channelSnare }, issues),
                "A valid binding table must compile.");
        require(table.resolve(1, 20) == 0 && table.resolve(16, 20) == 0
                    && table.resolve(2, 21) == 1
                    && table.resolve(1, 21) == InstrumentControlBindingTable::invalidControlIndex,
                "Any-channel and exact-channel binding lookup must be deterministic.");
        require(table.destinationController(0) == 20 && table.destinationController(1) == 21,
                "The compiled audio destination must follow the authored binding source.");

        InstrumentControlRuntimeState state;
        state.prepare(table);
        require(state.currentValue(0) == 0.25 && state.currentValue(1) == 0.75,
                "Runtime control state must initialize from authored defaults.");
        require(state.applyMidi(1, 20, 127, table) && state.currentValue(0) == 1.0,
                "A mapped MIDI CC must update normalized control state.");
        require(!state.applyMidi(1, 21, 127, table),
                "An unmatched exact-channel CC must not update control state.");
        require(state.applyMidi(2, 21, 64, table) && state.currentValue(1) > 0.50
                    && state.currentValue(1) < 0.51,
                "An exact-channel MIDI CC must update its destination.");
        require(state.resetControl(0) && state.currentValue(0) == 0.25,
                "Per-control reset must restore the authored default.");

        auto rebound = anyKick;
        rebound.controllerNumber = 22;
        issues.clear();
        require(table.compile({ kick, snare }, { rebound, channelSnare }, issues)
                    && table.destinationController(0) == 22,
                "Manual rebinding must retarget the compiled audio destination.");

        auto conflicting = channelSnare;
        conflicting.id = "binding.kick.cc20.ch2";
        conflicting.controlId = snare.id;
        conflicting.controllerNumber = 20;
        conflicting.channelScope = {};
        issues.clear();
        require(!table.compile({ kick, snare }, { anyKick, conflicting }, issues)
                    && !issues.empty(),
                "Duplicate MIDI sources must be rejected before runtime publication.");

        std::cout << "Instrument control binding tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Instrument control binding tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
