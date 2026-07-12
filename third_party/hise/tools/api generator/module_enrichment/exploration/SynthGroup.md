# Synthesiser Group (SynthGroup) - C++ Exploration

**Source:** `hi_core/hi_dsp/modules/ModulatorSynthGroup.h`, `hi_core/hi_dsp/modules/ModulatorSynthGroup.cpp`
**Base class:** `ModulatorSynth`

## Signal Path

MIDI events arrive and are forwarded to the group's MIDI processor chain and then to all allowed child synths' modulation chains. On note-on, the group determines which children are active (all children in normal mode, only carrier+modulator in FM mode). For each active voice, the group renders all children with shared modulation: the group's Pitch Modulation values are multiplied onto each child's own pitch values, and unison detune/spread offsets are applied per unisono voice. After all children render into the voice buffer, the group's Gain Modulation is applied. Voice-level effects run on the per-voice buffer. All voices are then summed, master effects are applied, and the result is routed to output.

```
MIDI in -> MidiProcessorChain -> forward to children's modulation chains
    -> per voice:
        -> [FM path]: render modulator (no detune) -> modulator audio + 1.0 -> fmModBuffer
                      render carrier per unisono voice: pitch *= fmModBuffer, apply detune
        -> [normal path]: for each unisono voice, for each active child:
                          child pitch *= group pitch * detune multiplier
                          child renders -> mix into voice buffer with gain * balance * detune balance
        -> apply group Gain Modulation to voice buffer
        -> group voice effects
    -> sum all voices
    -> master effects
    -> output
```

## Gap Answers

### shared-modulation-mechanism: How does SynthGroup share modulation across child synths?

The group's Gain Modulation and Pitch Modulation chains are calculated once at the group level. For pitch: the group's pitch values are **multiplied** onto each child's own pitch values (the child's modulation chain runs first, then the group's values are multiplied in). For gain: the group's gain modulation is applied after all children have rendered into the voice buffer, as a per-sample multiply on both channels. This means the group's modulators (envelopes, LFOs) apply collectively to all children - one AHDSR on the group controls the volume envelope for the combined output.

Children's own modulation chains still run independently (via `calculateModulationValuesForVoice`), so the final result is child modulation * group modulation.

### fm-synthesis-implementation: How is FM synthesis implemented?

When EnableFM is on and both carrier/modulator indices are valid:

1. The FM **modulator** child renders first (single voice, no unison detune applied)
2. The modulator's left channel audio output is copied to `fmModBuffer`
3. The buffer is multiplied by the modulator's gain, then **1.0 is added** - centering the modulation around 1.0 (silence = no pitch change)
4. For each unisono voice, the **carrier** child renders with its pitch values multiplied by `fmModBuffer` - the modulator's audio directly scales the carrier's frequency
5. Only carrier and modulator children are active - `handleActiveStateForChildSynths` deactivates all other children

When `EnableFM` is off but `CarrierIndex` is set (not -1), the carrier is **soloed** - only that child plays, all others are silenced. When both indices are -1 and FM is off, all non-bypassed children play normally.

### unison-voice-rendering: How are unison voices rendered?

Detune distribution is linear across unisono voices:
- `normalizedVoiceIndex = unisonoIndex / (numUnisonoVoices - 1)` (0.0 to 1.0)
- `normalizedDetuneAmount = normalizedVoiceIndex * 2.0 - 1.0` (-1.0 to +1.0)
- `detuneOctaveAmount = detune * normalizedDetuneAmount * detuneModValue`
- The pitch multiplier is derived from `octaveRangeToPitchFactor(detuneOctaveAmount)`

So for 4 voices at 12st detune: voice offsets are -12, -4, +4, +12 semitones.

Stereo spread: `detuneBalanceAmount = normalizedDetuneAmount * 100.0 * balance * spreadModValue`. Negatively-detuned voices pan left, positively-detuned voices pan right. Spread of 1.0 gives full stereo width.

Gain compensation: `gainFactor = 1.0 / sqrt(numUnisonoVoices)` - equal-power scaling.

Random start offsets: When unison > 1, each child voice gets a random start offset up to 441 samples (~10ms at 44.1kHz) to avoid phase cancellation.

### unison-cpu-scaling: Does UnisonoVoiceAmount multiply the actual voice count?

Yes. Each group voice starts `numUnisonoVoices` child voices per active child synth. The voice limit is automatically reduced: `voiceLimit = NUM_POLYPHONIC_VOICES / unisonoVoiceAmount`. With 256 polyphonic voices and 8 unisono voices: maximum 32 simultaneous notes. With 16 unisono: maximum 16 notes. This is a direct CPU multiplier.

### child-constrainer-meaning: Why are specific types excluded as children?

The forbidden types (`ModulatorSynthChain`, `ModulatorSynthGroup`, `GlobalModulatorContainer`, `MacroModulationSource`) are all containers or utility processors that would break the voice-level rendering model. The header explicitly states: "ModulatorSynthGroups can't be nested. The child processors only collect modulation from their immediate parent." SynthChain has its own voice management and master-level processing. GlobalModulatorContainer and MacroModulationSource are modulation utilities, not sound sources.

### child-fx-constrainer: What does VoiceEffect constrainer on children mean?

The `child_fx_constrainer` restricts what effects can be placed in the FX chains of child synths inside the group. Only polyphonic voice-level effects are allowed: PolyphonicFilter, HarmonicFilter, StereoEffect, JavascriptPolyphonicEffect, PolyshapeFX, HardcodedPolyphonicFX, and NoiseGrainPlayer. When a child synth is added to the group, any non-voice-effect processors are automatically removed with the message "Removed non-polyphonic FX - A child of a synth group can only render polyphonic effects." This is because children are rendered at the voice level inside the group voice's `calculateBlock`, not at the master level.

### force-mono-placement: Where does ForceMono apply?

ForceMono applies inside the per-voice rendering, at the point where each child voice's output is mixed into the group voice buffer. It sums L+R channels of each child voice and averages (multiply by 0.5), then writes the mono signal using the detune balance gain factors. This means the child's internal stereo content is collapsed to mono **before** unison stereo spread is applied. Unison voices still spread across the stereo field with ForceMono on - only the per-child stereo content is collapsed.

### kill-second-voices: What does KillSecondVoices do?

When enabled (default), KillSecondVoices enforces per-note monophony: if the same MIDI note number is retriggered, the older voice playing that note is killed. It compares uptimes to identify the older voice. This prevents voice buildup from rapid retriggering of the same note, which is important in a group context where each note triggers voices across multiple child synths (and potentially multiple unisono copies). Without it, rapid note repetition quickly exhausts the voice pool.

When disabled, multiple instances of the same note can play simultaneously (stacking behaviour).

### carrier-modulator-index-minus-one: What does -1 mean for CarrierIndex/ModulatorIndex?

-1 means "not set". `getFMCarrier()` returns `nullptr` if `carrierIndex <= 0`, and `fmCorrectlySetup` is set to `false` if either index is -1 or if they are the same index. When FM is enabled but the carrier is -1, the state message shows "The carrier synthesiser is not valid". When FM is off and CarrierIndex is not -1, only that carrier child plays (solo mode). When both are -1 and FM is off, all non-bypassed children play.

### rendering-order: What is the full rendering order?

1. MIDI events forwarded to group and children's modulation chains
2. Group's modulation chains render (Gain, Pitch, Detune, Spread)
3. All children's modulation chains render (`preVoiceRendering`)
4. Per voice: voice buffer cleared
5. Per voice: Detune/Spread mod values sampled
6. Per voice, per unisono index, per active child: calculate detune multipliers, multiply group pitch onto child pitch, child voice renders, mix into voice buffer
7. Per voice: group Gain Modulation applied to voice buffer
8. Per voice: group voice effects applied
9. All voices summed into group internal buffer
10. Master effects applied
11. Output routing with static Gain * Balance

### ui-components: Does SynthGroup have a custom editor or FloatingTile?

`createEditor()` returns `GroupBody`, a Projucer-generated component with: carrier selector combo, modulator selector combo, FM enable button, FM state label, unisono slider, detune slider (with mod display), spread slider (with mod display), force mono button, fade time editor, and voice amount editor. No FloatingTile content types are registered.

## Processing Chain Detail

1. **MIDI processing** - Forward events to group and all children's modulation chains. CPU: negligible.
2. **Modulation chain rendering** - Group's Gain, Pitch, Detune, Spread chains calculate values. CPU: depends on modulators added.
3. **Children's modulation rendering** - Each child's own modulation chains render. CPU: depends on children's modulators.
4. **Per-voice rendering** (repeated per active voice):
   a. **Clear voice buffer** - CPU: negligible.
   b. **Detune multiplier calculation** - Linear distribution of pitch offsets and stereo positions. CPU: negligible.
   c. **Child voice rendering** - Each child calculates its audio block. CPU: depends on child type (medium for sine, high for sampler).
   d. **ForceMono summing** (if enabled) - Sum L+R, average. CPU: low.
   e. **Mix into voice buffer** - Copy/add with gain * balance * detune factors. CPU: low.
   f. **FM modulation** (if enabled) - Additional modulator rendering + pitch multiplication. CPU: medium (extra voice render + per-sample multiply).
5. **Group gain modulation** - Multiply gain mod values onto voice buffer. CPU: low.
6. **Group voice effects** - Voice-level FX on per-voice buffer. CPU: depends on effects added.
7. **Voice summation** - Sum all active voices. CPU: low.
8. **Master effects** - Group-level master FX. CPU: depends on effects added.

## Modulation Points

- **Gain Modulation** (chainIndex 1): All modulator types allowed. Applied per-voice after all children have rendered into the voice buffer. Supports envelopes and voice-start modulators. Mode: ScaleOnly.
- **Pitch Modulation** (chainIndex 2): All modulator types allowed. Multiplied onto each child's own pitch values per-voice. This is the core shared modulation feature.
- **Detune Modulation** (chainIndex 4): Scales the UnisonoDetune parameter value. Auto-bypassed when unisono voice count is 1.
- **Spread Modulation** (chainIndex 5): Scales the UnisonoSpread parameter value. Auto-bypassed when unisono voice count is 1.

## Conditional Behaviour

- **EnableFM = On with valid indices**: Only carrier and modulator children render. FM modulator renders first (single voice, no detune). Its audio output (scaled by gain, +1.0) multiplies the carrier's pitch values. All other children are silenced.
- **EnableFM = Off, CarrierIndex != -1**: Carrier child is soloed. Only that child renders; all others silenced.
- **EnableFM = Off, CarrierIndex = -1**: Normal mode. All non-bypassed children render.
- **UnisonoVoiceAmount = 1**: Single voice per note. Detune and Spread modulation chains are auto-bypassed.
- **UnisonoVoiceAmount > 1**: Multiple voices per note with pitch spread and stereo panning. Random start offsets applied. Voice limit reduced proportionally.
- **ForceMono = On**: Child stereo content collapsed to mono before unison spread. Unison stereo spread still applies.
- **KillSecondVoices = On**: Same-note retriggering kills the older voice. Off = stacking allowed.

## Interface Usage

No relevant interfaces beyond RoutingMatrix (skip - inferred from type/subtype).

## CPU Assessment

- **Baseline: low** - The container itself adds minimal CPU (buffer management, gain multiply, output routing).
- **Polyphonic: true** - Per-voice rendering with group-level modulation.
- **Scaling factors**:
  - `UnisonoVoiceAmount`: Direct CPU multiplier. 8 unisono = 8x the child rendering cost per note. Also reduces maximum polyphony proportionally.
  - `EnableFM`: Adds one extra voice render (FM modulator) plus per-sample pitch multiplication on the carrier.
  - Number of children: Each active child renders per voice per unisono copy.

## UI Components

`GroupBody` editor with: carrier/modulator combo selectors, FM enable button, FM state label, unisono/detune/spread sliders, force mono button, fade time and voice amount editors. No FloatingTile content types registered.

## Notes

- The FM synthesis model is additive-over-pitch: the modulator's audio output is added to 1.0 (centering around "no modulation") and then multiplied into the carrier's pitch values. This means the modulator's amplitude directly controls the FM depth, and the modulator's frequency determines the modulation rate. This is classic FM synthesis.
- The FM modulator always renders without detune (single voice at `voiceIndex`, not unisono-indexed). Only the carrier gets unison detune applied.
- When FM is correctly set up, only two children (carrier and modulator) are active. All other children are deactivated and produce no sound.
- The carrier solo behaviour (FM off, CarrierIndex set) is a useful but non-obvious feature for quickly A/B-ing child synths.
- The `child_fx_constrainer` (`VoiceEffectConstrainer`) prevents master effects in child FX chains because children render at the voice level. This is enforced at add-time with automatic removal of incompatible effects.
- Detune and Spread modulation chains are auto-bypassed when `UnisonoVoiceAmount = 1`, avoiding unnecessary modulation calculation.
