# Scriptnode Voice Killer - C++ Exploration

**Source:** `hi_scripting/scripting/ScriptProcessorModules.h`, `hi_scripting/scripting/ScriptProcessorModules.cpp`
**Base class:** `EnvelopeModulator`, `snex::Types::VoiceResetter`

## Signal Path

The ScriptnodeVoiceKiller is a structural envelope modulator with no DSP processing. Its signal path is:

noteOn -> set voice state active -> output constant 1.0 modulation
scriptnode network signals gate close -> onVoiceReset -> set voice state inactive -> voice killed

The module acts as a bridge between the scriptnode voice management system and HISE's voice lifecycle. It does not shape the modulation signal - it always outputs 1.0 (unity gain). Its sole purpose is to track which voices are active and respond to voice reset signals from the scriptnode network.

**Initialisation flow:**
1. On construction, the module schedules a deferred initialisation call (300ms delay)
2. During initialisation, it checks whether it is placed in the gain modulation chain of a parent synth
3. If the parent synth implements DspNetwork::Holder, it registers itself as the voice killer via `setVoiceKillerToUse`
4. The DspNetwork propagates the voice killer reference to all active networks

**Voice lifecycle:**
- `startVoice`: Sets the voice state to active, returns 1.0
- `calculateBlock`: Fills the modulation buffer with 1.0 (constant unity)
- `onVoiceReset`: Called by the scriptnode network when the gate closes. Sets the voice state to inactive
- `isPlaying`: Returns whether the voice state is active
- `stopVoice`: Empty implementation (voice stopping is handled via onVoiceReset)
- `reset`: Sets voice state to inactive

## Gap Answers

### gate-monitoring-mechanism: How does ScriptnodeVoiceKiller monitor the scriptnode envelope's gate signal?

The ScriptnodeVoiceKiller does not poll the gate signal. Instead, it implements the VoiceResetter interface. During initialisation, it registers itself with the parent synth's DspNetwork::Holder via `setVoiceKillerToUse`. The DspNetwork then holds a weak reference to the voice killer and calls `onVoiceReset` when a scriptnode envelope node's gate closes. This is a push-based mechanism - the scriptnode network notifies the voice killer, not the other way around.

### voice-killing-trigger: What triggers voice killing?

Voice killing is push-based. The scriptnode network calls `onVoiceReset(false, voiceIndex)` when an envelope node's gate signal reaches zero. This sets the per-voice `active` atomic to false. The HISE voice management system then checks `isPlaying()` which returns the active state, and kills the voice when it returns false.

For an "all voices" reset, `onVoiceReset(true, _)` sets all voice states to inactive.

### calculateBlock-output: What does calculateBlock output?

calculateBlock fills the entire modulation buffer with 1.0 (unity gain). The module contributes no shaping to the modulation chain - it is purely structural. Its modulation output is always unity, so it does not attenuate the signal in any way.

### parent-placement: Does the ScriptnodeVoiceKiller need to be in the gain modulation chain?

During initialisation, the module checks whether its parent modulator chain is the GainModulation chain of the parent synth. If it is in the gain chain, it registers itself as the voice killer. If placed elsewhere, it sets `initialised = true` and does nothing further - effectively becoming inert.

So the module must be placed in the **Gain Modulation** chain of the same sound generator that hosts the scriptnode envelope to function correctly.

### monophonic-retrigger-usage: Do Monophonic and Retrigger parameters have meaningful effect?

These parameters are inherited from the EnvelopeModulator base class. The `setInternalAttribute` and `getAttribute` methods delegate directly to `EnvelopeModulator::setInternalAttribute/getAttribute`. The Monophonic parameter controls whether the envelope runs in monophonic mode (shared state). The Retrigger parameter controls whether the envelope restarts when a new note arrives in monophonic mode. These are functional - the base class uses them to control voice rendering mode. However, since the module always outputs 1.0, their practical effect is limited to controlling voice management behaviour rather than modulation shape.

## Processing Chain Detail

1. **Voice start** (per-voice, negligible): Set voice active state to true, return 1.0
2. **Block calculation** (per-voice, negligible): Fill modulation buffer with 1.0
3. **Voice reset callback** (per-voice, negligible): Set voice active state to false when scriptnode signals gate close

## Conditional Behaviour

No conditional processing paths. The module always outputs 1.0 regardless of parameter settings. The only conditional logic is during initialisation (checking whether the module is in the gain chain and whether the parent synth implements DspNetwork::Holder).

## CPU Assessment

**Baseline: negligible.** The module performs only a buffer fill with a constant value and atomic boolean operations. No DSP computation, no modulation chain evaluation, no sample-by-sample processing.

## UI Components

Uses EmptyProcessorEditorBody - no custom editor or FloatingTile.

## Notes

- The 300ms deferred initialisation is necessary because the module tree may not be fully constructed when the voice killer is created
- The module has zero internal chains (getNumInternalChains returns 0) and zero child processors
- handleHiseEvent is empty - the module does not process MIDI events directly
- stopVoice is empty - voice lifecycle is managed entirely through onVoiceReset
