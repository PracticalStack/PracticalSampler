# Hardcoded Synthesiser - C++ Exploration

**Source:** `hi_core/hi_modules/hardcoded/HardcodedModules.h`, `HardcodedModules.cpp`, `HardcodedModuleBase.h`, `HardcodedModuleBase.cpp`
**Base class:** `ModulatorSynth`, `HardcodedSwappableEffect`

## Signal Path

MIDI in -> voice allocation -> [compiled network per voice] -> gain modulation -> FX chain -> audio out

The module is a polyphonic sound generator that runs a compiled C++ scriptnode network per voice. The voice rendering pipeline:
1. On note-on: preStartVoice marks the voice for initialisation
2. Voice::calculateBlock: on first call after note-on, calls voiceData.startVoice to initialise per-voice state in the compiled network
3. Clears voice buffer
4. Processes through compiled network with chunked modulation (extra mod chains applied)
5. Applies gain modulation (per-voice gain values or constant gain)
6. Passes through the FX chain

## Gap Answers

### network-loading: How does the synthesiser load a compiled network?

Same mechanism as HardcodedMasterFX - uses the HardcodedSwappableEffect base class. The factory provides compiled networks from a DLL (development) or static factory (export). setEffect() initialises the OpaqueNode in polyphonic mode (isPolyphonic() returns true because polyHandler is enabled).

Voice allocation uses the standard ModulatorSynth infrastructure with a custom Voice class. Each Voice holds a pointer back to the synthesiser and uses the shared OpaqueNode with a PolyHandler::ScopedVoiceSetter to select the active voice index during processing.

### voice-rendering: How does the voice rendering pipeline work?

Voice::calculateBlock:
1. If isVoiceStart flag is set, calls voiceData.startVoice() to initialise the voice in the compiled network, then clears the flag
2. Clears the voice buffer
3. Creates a RenderData object with the OpaqueNode, modulation properties, voice buffer pointers, start sample, and num samples
4. Sets the voice index via PolyHandler::ScopedVoiceSetter
5. Calls extraModSources.processChunkedWithModulation(rd) which evaluates modulation chains and processes the network in chunks
6. Applies gain modulation: either per-voice gain values (from gain modulation chain) or constant gain
7. Calls effectChain->renderVoice() for the FX chain

The voice is reset via Voice::resetVoice() which calls voiceData.reset(voiceIndex).

### parameter-exposure: How are parameters exposed alongside fixed parameters?

getParameterOffset() returns ModulatorSynth::Parameters::numModulatorSynthParameters (which is 4: Gain, Balance, VoiceLimit, KillFadeTime). Network parameters are accessed at indices starting from this offset.

getAttribute/setAttribute check if the index is below the offset - if so, they delegate to ModulatorSynth; otherwise they subtract the offset and call getHardcodedAttribute/setHardcodedAttribute.

The getIllegalParameterIds() method prevents network parameters from using reserved names: Gain, Balance, VoiceLimit, KillFadeTime, IconColour.

### modulation-chain-mapping: How do extra modulation chains work?

NUM_HARDCODED_SYNTH_MODS controls how many extra modulation chains are created (beyond the standard Gain and Pitch chains from ModulatorSynth). These are named "Extra1", "Extra2", etc. and initialised at chain index offset 2 (after Gain=0, Pitch=1... actually modChains index 2 onwards).

The extra chains are coloured grey (0xFF888888) by default but their ID and colour are updated when a network is loaded to match the parameter they modulate.

### complex-data-exposure: How are complex data types exposed?

Same mechanism as HardcodedMasterFX via the HardcodedSwappableEffect base class. Tables, SliderPacks, AudioFiles, and DisplayBuffers are created on demand and synchronised with the compiled network via DataWithListener objects.

### compiled-vs-interpreted: How does this differ from ScriptSynth?

ScriptSynth loads an XML scriptnode network and interprets it. HardcodedSynth loads a compiled C++ version from a DLL/static factory. Both share the same voice architecture (ModulatorSynth base), FX chain, gain/pitch modulation, but HardcodedSynth eliminates per-node interpretation overhead.

## Processing Chain Detail

1. **Voice start** (negligible) - initialises per-voice state in the compiled network via VoiceDataStack
2. **Buffer clear** (negligible) - zeros the voice buffer
3. **Extra modulation chains** (negligible to low) - evaluates and applies chunked modulation
4. **Compiled network processing** (depends on network) - per-voice audio generation via OpaqueNode
5. **Gain modulation** (negligible) - multiplies voice buffer by gain modulation values
6. **FX chain** (depends on effects) - voice effect processing

## Modulation Points

- **Gain Modulation** (chainIndex 1): Standard ModulatorSynth gain chain, multiplies voice output
- **Pitch Modulation** (chainIndex 2): Standard ModulatorSynth pitch chain, connected to compiled network via runtime targets
- **Extra1, Extra2, ...** (chainIndex 2+): Mapped to compiled network parameters via ModulationProperties

## CPU Assessment

- Framework overhead: negligible (voice management, gain multiply)
- Actual cost: determined by compiled network + FX chain
- Polyphonic: scales with voice count
- Baseline tier: negligible (framework only)

## Notes

- Note-on events are handled in preStartVoice (not forwarded directly to the network) - the voice start is deferred to the first calculateBlock call
- Other MIDI events (note-off, CC, etc.) are forwarded to the compiled network via voiceData.handleHiseEvent
- The voice buffer channel count is set from the compiled network's channel count
