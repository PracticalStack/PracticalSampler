# Polyphonic Script FX - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h` (line 670-803), `hi_scripting/scripting/ScriptProcessorModules.cpp` (line 499-758)
**Base class:** `VoiceEffectProcessor`, `JavascriptProcessor`, `ProcessorWithScriptingContent`

## Signal Path

PolyScriptFX (JavascriptPolyphonicEffect) is a per-voice effect that processes each voice independently through a scriptnode DSP network.

Per-voice: startVoice (init network voice) -> renderVoice (network process per voice with modulation) -> check suspension

The renderVoice method is the core path:
1. Pre-voice rendering setup
2. Set voice index via DspNetwork::VoiceSetter
3. Check pre-suspension (if voice is silent, skip processing)
4. Process through network root node with extra modulation chains
5. Check post-suspension
6. Update tailing status based on whether the voice is still active

Note: The `applyEffect()` method is intentionally empty (asserts false) - all processing happens in `renderVoice()`.

## Gap Answers

### network-loading-mechanism

Same as other Script* modules: inherits DspNetwork::Holder. Network created in onInit callback. Only onInit and onControl callbacks are available (no processBlock - the network handles all audio processing).

### voice-rendering

Each voice is processed independently with its own voice state:
1. `preVoiceRendering()` is called for setup
2. The network connection lock is acquired (read lock)
3. A `VoiceSetter` sets the current voice index in the network's poly handler
4. Pre-suspension check: if the voice buffer is silent, processing can be skipped
5. The network root node processes the audio via `processChunkedWithModulation` which handles extra modulation chain application
6. Post-suspension check for tailing detection

### voice-lifecycle

- **startVoice**: Calls VoiceEffectProcessor::startVoice, then voiceData.startVoice() to initialise the voice in the network
- **stopVoice**: Empty implementation (no-op) - the voice continues until reset
- **reset**: Calls VoiceEffectProcessor::reset, then voiceData.reset() to clean up network voice state
- **handleHiseEvent**: Non-noteOn events are forwarded to the network via voiceData.handleHiseEvent

### parameter-exposure

No fixed parameters. All parameters come from the loaded network via getCurrentNetworkParameterHandler.

### complex-data-routing

Same mechanism as other Script* modules.

### modulation-chain-support

PolyScriptFX supports extra modulation chains configured via HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS (default: 0). These are managed by ExtraModulatorRuntimeTargetSource and connected to network parameters at runtime. The pitch chain from the parent synth is also connected via connectToRuntimeTargets.

### tail-and-suspension

- **hasTail**: Returns true if the active network reports a tail, false otherwise
- **isSuspendedOnSilence**: Returns true if the network is not suspended on silence (inverted logic - the network controls this)
- **Voice suspension**: Pre/post suspension checks in renderVoice allow skipping processing for silent voices. The tailing flag is updated based on whether the voice is still tracked in voiceData.

## Processing Chain Detail

1. **Voice start** (startVoice): Initialise voice in network (negligible)
2. **Voice render** (renderVoice):
   - Pre-voice rendering setup (negligible)
   - Voice setter (negligible)
   - Pre-suspension check (negligible)
   - Network process with modulation chunks (depends on network)
   - Post-suspension check (negligible)

## Modulation Points

Extra modulation chains (when configured) modulate network parameters. The parent synth's pitch chain is also connected to the network's runtime targets.

## Interface Usage

Same as other Script* modules.

## CPU Assessment

- **Baseline**: Cannot be determined - depends on loaded network
- **Polyphonic**: true - cost scales linearly with active voices
- **Voice suspension**: Provides CPU savings for silent voices

## Notes

Unlike ScriptFX and ScriptTimeVariantModulator, PolyScriptFX has no HISEScript audio processing fallback. Only onInit and onControl callbacks are available. All audio processing must come from the scriptnode network.
