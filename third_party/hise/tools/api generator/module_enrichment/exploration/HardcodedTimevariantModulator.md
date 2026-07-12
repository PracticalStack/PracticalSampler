# Hardcoded Time Variant Modulator - C++ Exploration

**Source:** `hi_core/hi_modules/hardcoded/HardcodedModules.h`, `HardcodedModules.cpp`, `HardcodedModuleBase.h`, `HardcodedModuleBase.cpp`
**Base class:** `TimeVariantModulator`, `HardcodedSwappableEffect`

## Signal Path

[continuous processing at control rate] -> modulation output

The module runs a compiled C++ network monophonically at control rate to produce a continuous modulation signal.

## Gap Answers

### network-loading: How does the time-variant modulator load and run a compiled network?

Same HardcodedSwappableEffect mechanism. The network is loaded via setEffect(). The OpaqueNode is initialised in monophonic mode (isPolyphonic() returns false). numChannelsToRender is set to 1 in the constructor.

### parameter-exposure: How are parameters exposed?

This module has no fixed parameters (no parameter offset). All parameters come directly from the compiled network. The Intensity parameter is added to the illegal parameter IDs list to prevent conflicts with the base class.

### control-rate: Does the modulator run at control rate?

Yes. prepareOpaqueNode sets blockSize to largestBlockSize / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR and sampleRate to sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR.

### midi-handling: Does the module forward MIDI events?

Yes. handleHiseEvent creates a copy of the HiseEvent and calls opaqueNode->handleHiseEvent(copy). This allows the compiled network to respond to MIDI events (e.g., for MIDI-reactive LFOs or other modulation sources that need event input).

### complex-data-exposure: How are complex data types exposed?

Same mechanism as all hardcoded modules via HardcodedSwappableEffect base class.

## Processing Chain Detail

1. **Control rate processing** (depends on network) - calculateBlock clears the modulation buffer, creates a ProcessDataDyn pointing to it, and calls opaqueNode->process(). The network fills the buffer with modulation values.

## CPU Assessment

- Runs monophonically at control rate
- Framework overhead: negligible
- Actual cost depends on network
- Baseline tier: negligible (framework only)

## Notes

- Unlike HardcodedMasterFX, MIDI events ARE forwarded to the compiled network
- The channel count check enforces numChannels == 1
- No modulation chains are available (no child processors)
