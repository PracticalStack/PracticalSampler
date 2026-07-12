# Container (SynthChain) - C++ Exploration

**Source:** `hi_core/hi_dsp/modules/ModulatorSynthChain.h`, `hi_core/hi_dsp/modules/ModulatorSynthChain.cpp`
**Base class:** `ModulatorSynth`

## Signal Path

MIDI events arrive and are optionally filtered by channel (root chain only), then processed by the MIDI processor chain. Each non-bypassed child SoundGenerator renders additively into a shared internal buffer. After all children have rendered, monophonic gain modulation is applied to the summed buffer. The effect chain then processes in two stages: voice-level effects (forced monophonic) followed by master effects. Finally, the static Gain parameter and Balance are applied when copying to the output buffer.

```
MIDI in -> [root: channel filter] -> MidiProcessorChain
    -> for each child: render additively into internalBuffer
    -> forward controller/pitchwheel to chain-level handlers
    -> monophonic gain modulation (multiply onto internalBuffer)
    -> effectChain->renderNextBlock (voice FX, forced monophonic)
    -> effectChain->renderMasterEffects (master FX)
    -> apply static Gain * Balance -> output buffer
```

## Gap Answers

### container-rendering: How does SynthChain render its children?

SynthChain iterates `synths` (an `OwnedArray<ModulatorSynth>`) in `renderNextBlockWithModulators`. Each non-bypassed child's `renderNextBlockWithModulators` is called with the shared `internalBuffer`. Children render additively - each child adds its output to the buffer via `FloatVectorOperations::addWithMultiply` at the end of `ModulatorSynth::renderNextBlockWithModulators`. The mixing order follows the array order (top to bottom in the module tree). The buffer is cleared at the start via `initRenderCallback`.

### gain-modulation-scope: Is the gain modulation post-mix and monophonic?

Confirmed. The gain modulation chain is constrained to `TimeVariantModulator` types only via `NoMidiInputConstrainer`. It is calculated monophonically (`calculateMonophonicModulationValues`) and applied in `postVoiceRendering` by multiplying the monophonic modulation values onto all channels of `internalBuffer` - after all children have rendered. The static Gain parameter value is applied separately in the final output routing step (not via the modulation chain).

### child-constrainer: Is there a practical limit on nesting depth?

The constrainer is `*` (any SoundGenerator). There is no explicit depth limit in the code. SynthChain can be nested inside SynthChain. Each nested chain manages its own children, voices, and FX chain independently. The practical limits are CPU and the 256-voice total allocation per chain.

### fx-chain-placement: Where does the FX chain apply?

The FX chain runs in two stages, both after gain modulation:
1. `effectChain->renderNextBlock()` in `postVoiceRendering` - processes any voice-level effects (forced monophonic via `setForceMonophonicProcessingOfPolyphonicEffects(true)`)
2. `effectChain->renderMasterEffects()` - processes master effects (reverb, delay, convolution, etc.)

Both run after all children have been summed and gain modulation applied. The static Gain + Balance are applied after the FX chain, in the final output copy.

### voice-limit-distribution: How does VoiceLimit work across children?

Each child synth manages its own voice pool independently. `ModulatorSynthChain::getNumActiveVoices()` sums across all children. The chain's VoiceLimit parameter affects the number of voices allocated when creating child synths through the factory. Each child enforces its own voice limit via `handleVoiceLimit()` and `killLastVoice()`. There is no shared voice pool at the chain level.

### root-chain-special: Is the root SynthChain treated differently?

Yes, the root chain (where `getMainController()->getMainSynthChain() == this`) has several unique behaviours:
- **MIDI channel filtering**: Only the root chain filters events by active channel mask
- **Host info events**: `handleHostInfoHiseEvents()` only runs on root
- **Multi-channel buffer management**: Root updates the multi-channel buffer count
- **Preset serialisation extras**: Root saves package name, macro controls, MIDI automation handler, MPE data
- **Peak metering**: Only root updates peak display by default
- **Profiling**: Only root iterates all processors for profiling
- **Reset identity**: On reset, root sets its ID to "Master Chain"

Nested SynthChains behave like simple mixer containers without these global responsibilities.

### ui-components: Does SynthChain have a custom editor or FloatingTile?

`createEditor()` returns `EmptyProcessorEditorBody` - no custom editor body. The header file `ModulatorSynthChainBody.h` is empty. No FloatingTile content types are registered specifically for SynthChain. The UI is handled through the generic processor editor and FloatingTile system.

## Processing Chain Detail

1. **MIDI channel filter** (root only) - Marks events on disabled channels as ignored. CPU: negligible.
2. **Clear internal buffer** - `initRenderCallback()`. CPU: negligible.
3. **MIDI processor chain** - `processHiseEventBuffer` runs the MidiProcessorChain. CPU: depends on MIDI processors added.
4. **Child rendering loop** - Each child `renderNextBlockWithModulators` adds to internalBuffer. CPU: depends entirely on children.
5. **Controller/pitchwheel forwarding** - Forwards only controller and pitchwheel events to chain-level handlers. CPU: negligible.
6. **Monophonic gain modulation** - `calculateMonophonicModulationValues` then multiply onto all channels. CPU: low (per-block multiply).
7. **Voice FX (forced monophonic)** - `effectChain->renderNextBlock`. CPU: depends on effects added.
8. **Master FX** - `effectChain->renderMasterEffects`. CPU: depends on effects added.
9. **Output routing** - Multiply by static Gain * Balance, add to output buffer. CPU: negligible.

## Modulation Points

- **Gain Modulation** (chainIndex 1): Monophonic, TimeVariantModulator only. Applied post-mix as a per-sample multiply on the summed internal buffer. The modulation mode is `ScaleOnly`.
- **Pitch Modulation** (chainIndex 2): Disabled and bypassed in the constructor. Not applied.

## Conditional Behaviour

- **Root vs nested**: Root chain performs MIDI channel filtering, host info handling, and additional serialisation. Nested chains skip all of these.
- **FRONTEND_IS_PLUGIN**: An alternate rendering path exists when the exported plugin acts as an effect plugin. The effect chain runs on the incoming host audio buffer, and child sound generators optionally render into an internal buffer. This is only relevant to exported FX plugins, not normal instrument mode.

## Vestigial / Notable

No vestigial parameters. All four parameters (Gain, Balance, VoiceLimit, KillFadeTime) are functional. The Pitch Modulation chain is intentionally disabled and bypassed - not vestigial, but a deliberate design choice since child synths have their own pitch modulation.

## CPU Assessment

- **Baseline: negligible** - The container itself adds negligible CPU. It clears a buffer, iterates children, does a per-block gain multiply, and copies to output. All real CPU cost comes from children and effects.
- **Scaling factors**: None on the container itself. CPU scales with number of children and complexity of effects.
- **Polyphonic**: false (the container processes monophonically; children handle their own voices)

## UI Components

None. `createEditor()` returns `EmptyProcessorEditorBody`. No FloatingTile content types.

## Notes

- `ModulatorSynthChain` also extends `MacroControlBroadcaster`, providing macro control support. The root chain hosts up to 8 macro controls that can be mapped to any parameter in the module tree.
- `getParentProcessor()` always returns `nullptr` for any `ModulatorSynthChain`, even nested ones. This is a structural quirk - they are always considered top-level processors.
- The `NoMidiInputConstrainer` used for both gain modulation and FX chain is defined within `ModulatorSynthChain` and reused by `ModulatorSynthGroup` (as `SynthGroupFXConstrainer`).
- The `FORCE_INPUT_CHANNELS` path allows instrument plugins to pass through host audio input alongside generated audio.
