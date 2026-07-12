# Hardcoded Polyphonic FX - C++ Exploration

**Source:** `hi_core/hi_modules/hardcoded/HardcodedModules.h`, `HardcodedModules.cpp`, `HardcodedModuleBase.h`, `HardcodedModuleBase.cpp`
**Base class:** `VoiceEffectProcessor`, `HardcodedSwappableEffect`

## Signal Path

Audio in (per voice) -> [compiled C++ network per voice] -> audio out (per voice)

The module runs a compiled C++ scriptnode network as a per-voice effect processor with independent state per voice.

## Gap Answers

### network-loading: How does the polyphonic FX load a compiled network?

Same HardcodedSwappableEffect mechanism. The OpaqueNode is initialised in polyphonic mode (isPolyphonic() returns true). The polyHandler manages voice state.

### voice-lifecycle: How does voice management work?

- **startVoice(voiceIndex, event)**: Calls voiceStack.startVoice() to initialise per-voice state in the compiled network for the given voice index.
- **applyEffect(voiceIndex, buffer, startSample, numSamples)**: Sets voice index via PolyHandler::ScopedVoiceSetter, creates RenderData, checks pre-suspension, processes via extraModSources.processChunkedWithModulation, checks post-suspension, updates display values. Sets isTailing based on whether the voice is still in the voiceStack.
- **reset(voiceIndex)**: Calls VoiceEffectProcessor::reset() and voiceStack.reset(voiceIndex).
- **handleHiseEvent**: Forwards events to voiceStack.handleHiseEvent (except note-on, which is handled by startVoice).

### parameter-exposure: How are parameters exposed?

No fixed parameters (no parameter offset). All parameters come directly from the compiled network via getHardcodedAttribute/setHardcodedAttribute.

### modulation-chain-mapping: How do modulation chains work?

NUM_HARDCODED_POLY_FX_MODS controls the number of modulation chain slots. These are named "P1 Modulation", "P2 Modulation", etc. The ExtraModulatorRuntimeTargetSource maps them to compiled network parameters via ModulationProperties.

During rendering, renderVoice calls preVoiceRendering (if mods are enabled) before applyEffect. The modulation is applied via processChunkedWithModulation within applyEffect.

### complex-data-exposure: How are complex data types exposed?

Same mechanism as all hardcoded modules.

### tail-handling: How does tail/voice reset work?

The VoiceResetter interface allows the compiled network to signal voice reset. isVoiceResetActive() returns true when the OpaqueNode has a tail (hasHardcodedTail delegates to opaqueNode->hasTail()). When the network signals reset, onVoiceReset either calls allNotesOff (for all voices) or voiceStack.reset(voiceIndex) for a single voice.

The isTailing flag is set in applyEffect based on whether the voiceStack still contains the voice index, allowing the effect to continue processing after the voice's sound generator has stopped.

## Processing Chain Detail

1. **Voice start** (negligible) - initialises per-voice state in compiled network
2. **Pre-suspension check** (negligible) - checks if voice can be suspended
3. **Modulation chain evaluation** (negligible to low) - evaluates extra modulation chains
4. **Compiled network processing** (depends on network) - per-voice effect via OpaqueNode
5. **Post-suspension check** (negligible) - marks voice for suspension if output is silent
6. **Display update** (negligible) - routing matrix display values

## CPU Assessment

- Per-voice processing: scales with active voice count
- Framework overhead: negligible
- Actual cost: depends on compiled network
- Baseline tier: negligible (framework only)

## Notes

- The routing matrix is restricted to only-enabling (setOnlyEnablingAllowed(true))
- Channel count must match between routing matrix connections and compiled network
- renderNextBlock has an empty implementation (all processing goes through applyEffect per voice)
