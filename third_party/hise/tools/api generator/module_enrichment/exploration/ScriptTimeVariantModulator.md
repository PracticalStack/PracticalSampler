# Script Time Variant Modulator - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h` (line 284-378), `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 1308-1567)
**Base class:** `TimeVariantModulator`, `JavascriptProcessor`, `ProcessorWithScriptingContent`

## Signal Path

ScriptTimeVariantModulator (JavascriptTimeVariantModulator) is a dual-mode monophonic modulator that generates continuous modulation values.

**Network mode**: internal buffer cleared -> network root node process (1 channel) -> clip output to 0-1
**Script mode**: buffer passed to processBlock HISEScript callback

The module checks `getActiveNetwork()` first. If a network is active, it processes through the network. Otherwise falls back to the processBlock HISEScript callback.

In network mode, the output is explicitly clipped to the 0.0-1.0 range via FloatVectorOperations::clip. In script mode, no automatic clipping is applied.

## Gap Answers

### network-loading-mechanism

Same as other Script* modules: inherits DspNetwork::Holder. Network created in onInit callback via DspNetwork creation.

### dual-mode-operation

Yes, ScriptTimeVariantModulator supports both modes:
1. **Scriptnode mode**: When getActiveNetwork() returns a network. Processing through network root node with 1 channel.
2. **HISEScript mode**: When no network but processBlock callback has code. Buffer is wrapped in a VariantBuffer and passed to the processBlock callback. Additional callbacks available: prepareToPlay, onNoteOn, onNoteOff, onController.

The HISEScript mode offers more callbacks (7 total: onInit, prepareToPlay, processBlock, onNoteOn, onNoteOff, onController, onControl) than the network mode which only uses onInit and onControl.

### output-clipping

In network mode, the output is clipped to 0.0-1.0 via `FloatVectorOperations::clip(ptr, ptr, 0.0f, 1.0f, numSamples)`. This ensures the modulation output stays within the valid range regardless of what the network produces. In script mode, no automatic clipping is applied - the script is responsible for producing values in range.

### parameter-exposure

No fixed parameters. All parameters come from either the loaded network (via networkParameterHandler) or the HISEScript content (via contentParameterHandler). The getCurrentNetworkParameterHandler selects the appropriate handler.

### complex-data-routing

Same mechanism as other Script* modules.

### midi-event-handling

In HISEScript mode, MIDI events are dispatched to the appropriate callbacks:
- noteOn events -> onNoteOn callback
- noteOff events -> onNoteOff callback  
- Controller events -> onController callback

In network mode, MIDI events are forwarded to the network via `n->getRootNode()->handleHiseEvent(c)` for all non-noteOn events (noteOn is skipped as the modulator is monophonic and doesn't need voice start handling).

## Processing Chain Detail

1. **Calculate block** (calculateBlock):
   - Network mode: clear buffer, process through network root node (1 channel), clip to 0-1 (depends on network)
   - Script mode: wrap buffer, execute processBlock callback (depends on script)
2. **MIDI handling** (handleHiseEvent): Forward events to network or script callbacks (negligible)

## Modulation Points

No modulation chains on this module. The "Intensity" parameter is listed as illegal (filtered from network parameters).

## Interface Usage

Same as other Script* modules.

## CPU Assessment

- **Baseline**: Cannot be determined - depends on loaded network/script
- **Polyphonic**: false (monophonic time-variant modulator)

## Notes

The "Intensity" parameter ID is added to the illegal parameter list, preventing network parameters with that name from being exposed (as Intensity is a built-in modulator property).
