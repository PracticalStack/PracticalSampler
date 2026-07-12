# Silent Synth - C++ Exploration

**Source:** `hi_core/hi_modules/synthesisers/synths/NoiseSynth.h`, `hi_core/hi_modules/synthesisers/synths/NoiseSynth.cpp`
**Base class:** `ModulatorSynth`

## Signal Path

SilentSynth is a sound generator that produces no audio but routes its voice buffer through an FX chain. The signal path per voice is:

noteOn -> clear voice buffer (silence) -> run FX chain on voice buffer -> mix into output

The voice rendering clears the buffer to zero, then immediately passes it to the effect chain. This means effects in the chain receive a silent input, which is useful for:
- Effects that generate their own audio (e.g. from sidechain inputs or send buses)
- Hosting effects that process external signals routed via the routing matrix

**Key characteristics:**
- `synthNeedsEnvelope()` returns false - no envelope is required
- Both Gain Modulation and Pitch Modulation chains are explicitly disabled
- The routing matrix allows resizing (`setAllowResizing(true)`) for multi-channel configurations
- `addProcessorsWhenEmpty()` is overridden to do nothing (no default child processors added)

## Gap Answers

### voice-rendering: What does the voice rendering do?

The SilentVoice::calculateBlock method clears the voice buffer to zero, then calls the effect chain's renderVoice. So the FX chain receives a completely silent buffer. This is intentional - the "Sound of silence" comment in the source confirms this is by design.

### fx-chain-usage: How does the FX chain operate on a silent voice buffer?

The FX chain processes the voice buffer after it has been cleared. The fx_constrainer is "*" meaning any effect type can be added. Since the input is silence, only effects that receive signal from elsewhere (via routing matrix, send effects, or sidechain) will produce meaningful output. Effects that process their input (like filters or delays) will pass through silence.

### envelope-requirement: Does SilentSynth require an envelope?

No. `synthNeedsEnvelope()` returns false, which means HISE will not warn the user about missing envelopes. This is because the module produces no audio - there is no amplitude to shape.

### routing-matrix: Does the routing matrix allow resizing?

Yes. The constructor calls `getMatrix().setAllowResizing(true)`, and the `numSourceChannelsChanged` method handles multi-channel reconfiguration. It resizes the internal buffer, prepares all voices for the new channel count, and propagates the channel configuration to all effects in the FX chain.

### check-release-behavior: How does voice release work?

SilentVoice overrides `checkRelease()`. If the voice is being killed (kill fade), it waits for the fade to reach silence before resetting. Otherwise, if the effect chain has no tailing polyphonic effects, the voice is reset immediately. If there are tailing poly effects (reverb tails, delay tails), the voice stays alive until those effects finish.

## Processing Chain Detail

1. **Voice buffer clear** (per-voice, negligible): Clear the voice buffer to zero
2. **FX chain render** (per-voice, variable): Run all effects in the FX chain on the voice buffer. CPU cost depends entirely on which effects are loaded
3. **Gain/Balance apply** (per-voice, negligible): Standard ModulatorSynth output stage (Gain, Balance)
4. **Voice check release** (per-voice, negligible): Check whether tailing poly effects are still active

## Conditional Behaviour

No parameter-driven conditional paths. The voice always renders silence and runs the FX chain. The `checkRelease` method has a conditional for tailing poly effects, but this is implicit based on the loaded effects.

## CPU Assessment

**Baseline: negligible** (with empty FX chain). The module itself does almost nothing - just a buffer clear. Actual CPU cost depends entirely on the effects loaded into the FX chain.

**Scaling factors:**
- FX chain contents: CPU scales with the number and complexity of loaded effects

## UI Components

Uses EmptyProcessorEditorBody - no custom editor or FloatingTile.

## Notes

- SilentSynth is defined in the same file as NoiseSynth (NoiseSynth.h/cpp), sharing the SilentSound and SilentVoice helper classes
- The SilentSound class matches all notes, channels, and velocities
- preVoiceRendering calls effectChain->preRenderCallback, ensuring effects are properly prepared before voice rendering
- The module disables gain and pitch modulation chains in both the metadata and the constructor, confirming these are intentionally non-functional
