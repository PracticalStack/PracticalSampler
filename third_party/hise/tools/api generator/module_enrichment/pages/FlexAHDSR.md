---
title: Flex AHDSR Envelope
moduleId: FlexAHDSR
type: Modulator
subtype: EnvelopeModulator
tags: [generator]
builderPath: b.Modulators.FlexAHDSR
screenshot: /images/v2/reference/audio-modules/flexahdsr.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: AHDSR, type: alternative, reason: "Simpler AHDSR with coefficient-based curves and a single shared decay/release shape" }
  - { id: SimpleEnvelope, type: alternative, reason: "Lighter-weight envelope with fewer stages and no curve control" }
  - { id: TableEnvelope, type: alternative, reason: "Table-driven envelope for arbitrary shapes that cannot be expressed as power curves" }
  - { id: "envelope.flex_ahdsr", type: scriptnode, reason: "Scriptnode equivalent with per-segment curve control and trigger/loop modes" }
commonMistakes:
  - title: "Trigger mode skips sustain entirely"
    wrong: "Using Trigger mode and expecting the envelope to hold at the sustain level"
    right: "Trigger mode plays through all stages as a one-shot - the sustain stage is skipped"
    explanation: "In Trigger mode the envelope advances immediately from decay to release without waiting for note-off. Use Note mode for standard sustain behaviour."
  - title: "Curve default is 0.5, not 0"
    wrong: "Assuming a curve value of 0 produces a linear shape"
    right: "A curve value of 0.5 is linear; 0 produces an extreme logarithmic curve"
    explanation: "The curve slider maps 0.5 to a linear ramp. Values below 0.5 produce convex shapes and values above 0.5 produce concave shapes."
  - title: "Envelope state not saved to presets by default"
    wrong: "Expecting user presets to restore the FlexAHDSR configuration automatically"
    right: "Call Engine.addModuleStateToUserPreset(\"envelopeId\") to include the envelope state in presets"
    explanation: "Without this call, preset loads will not restore envelope parameters that were changed via the drag interface."
  - title: "Hold stage has no curve drag handle"
    wrong: "Looking for a curve drag point on the Hold segment in the envelope display"
    right: "The Hold stage is a flat segment at the attack level - it has no curvature and only its duration can be adjusted"
    explanation: "Hold outputs a constant level (the AttackLevel value) for its duration. Only Attack, Decay, and Release stages have curve handles."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedEnvelopeModulator
  complexity: medium
  description: "Use the envelope.flex_ahdsr scriptnode node for identical behaviour inside a HardcodedEnvelopeModulator network"
llmRef: |
  Flex AHDSR Envelope (Modulator/EnvelopeModulator)

  A per-voice AHDSR envelope with independent power-curve shaping per stage (Attack, Hold, Decay, Sustain, Release) and three playback modes. Uses progress-based power curves rather than coefficient-based exponentials, giving direct control over each stage's curvature.

  Signal flow:
    noteOn -> evaluate mod chains -> Attack (curved) -> Hold (flat) -> Decay (curved) -> Sustain (flat) -> noteOff -> Release (curved) -> done
    Mode controls sustain behaviour: Note=hold at sustain, Trigger=skip sustain (one-shot), Loop=repeat attack-through-release while gate active.

  CPU: low, polyphonic. Processed at control rate (downsampled from audio rate). Linear curves use a fast path avoiding pow().

  Parameters:
    Attack (0-30000 ms, default 5 ms) - time to reach AttackLevel
    Hold (0-30000 ms, default 0 ms) - time held at AttackLevel before decay
    Decay (0-30000 ms, default 100 ms) - time to fall from AttackLevel to Sustain
    Sustain (0.0-1.0, default 0.5) - level held during note (linear amplitude, not dB)
    Release (0-30000 ms, default 300 ms) - time to fall to zero after note-off
    Mode (Trigger, Note, Loop, default Note) - playback behaviour
    AttackLevel (0.0-1.0, default 1.0) - peak level at end of attack (linear amplitude)
    AttackCurve (0.0-1.0, default 0.5) - attack curvature (0.5=linear)
    DecayCurve (0.0-1.0, default 0.5) - decay curvature (0.5=linear)
    ReleaseCurve (0.0-1.0, default 0.5) - release curvature (0.5=linear)
    Monophonic (Off/On, default dynamic) - forces single voice
    Retrigger (Off/On, default On) - restarts envelope on new note in monophonic mode

  Modulation chains (5):
    AttackTimeModulation (VoiceStart) - multiplies attack time
    AttackLevelModulation (VoiceStart) - multiplies attack and hold level
    DecayTimeModulation (VoiceStart) - multiplies decay time
    SustainLevelModulation (VoiceStart + TimeVariant) - multiplies sustain and decay target level
    ReleaseTimeModulation (VoiceStart) - multiplies release time

  Common mistakes:
    Trigger mode skips sustain entirely - use Note mode for standard AHDSR.
    Curve values: 0.5=linear, not 0=linear.
    Envelope state requires Engine.addModuleStateToUserPreset() for preset recall.

  Custom equivalent:
    scriptnode via HardcodedEnvelopeModulator using the envelope.flex_ahdsr node.

  See also:
    alternative AHDSR - simpler envelope with coefficient-based curves
    alternative SimpleEnvelope - lighter-weight envelope
    alternative TableEnvelope - table-driven arbitrary shapes
    [scriptnode] envelope.flex_ahdsr -- scriptnode equivalent with per-segment curve control and trigger/loop modes
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![Flex AHDSR Envelope screenshot](/images/v2/reference/audio-modules/flexahdsr.png)

The Flex AHDSR Envelope is a per-voice envelope modulator with independent power-curve shaping for each timed stage. Unlike the standard [AHDSR]($MODULES.AHDSR$) which uses coefficient-based exponential curves with a single shared decay/release shape, the FlexAHDSR gives separate curve control over Attack, Decay, and Release and uses a progress-based power-curve approach that maps directly to visual drag handles in the envelope display.

Three playback modes control how the envelope responds to note events. **Note** mode is the standard AHDSR behaviour where the sustain stage holds until note-off. **Trigger** mode plays through all stages as a one-shot, skipping sustain entirely - useful for percussive sounds or sound effects. **Loop** mode repeats the envelope from attack through release for as long as the note is held, creating rhythmic or tremolo-like effects. The Sustain and AttackLevel parameters are linear amplitude values (0-1), not decibel values.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Time to ramp from zero to AttackLevel"
      range: "0 - 30000 ms"
      default: "5 ms"
    Hold:
      desc: "Time held at AttackLevel before decay begins"
      range: "0 - 30000 ms"
      default: "0 ms"
    Decay:
      desc: "Time to fall from AttackLevel to Sustain level"
      range: "0 - 30000 ms"
      default: "100 ms"
    Sustain:
      desc: "Level maintained while the note is held (linear amplitude)"
      range: "0.0 - 1.0"
      default: "0.5"
    Release:
      desc: "Time to fall from the current level to zero after note-off"
      range: "0 - 30000 ms"
      default: "300 ms"
    Mode:
      desc: "Playback behaviour: Trigger (one-shot), Note (standard), Loop (repeating)"
      range: "Trigger, Note, Loop"
      default: "Note"
    AttackLevel:
      desc: "Peak level reached at the end of the attack stage (linear amplitude)"
      range: "0.0 - 1.0"
      default: "1.0"
    AttackCurve:
      desc: "Curvature of the attack stage (0.5 = linear)"
      range: "0.0 - 1.0"
      default: "0.5"
    DecayCurve:
      desc: "Curvature of the decay stage (0.5 = linear)"
      range: "0.0 - 1.0"
      default: "0.5"
    ReleaseCurve:
      desc: "Curvature of the release stage (0.5 = linear)"
      range: "0.0 - 1.0"
      default: "0.5"
  functions:
    powerCurve:
      desc: "Applies a power-curve exponent to the stage progress (0-1), shaping the interpolation between start and target levels"
    interpolate:
      desc: "Linearly interpolates between the previous level and the target level using the curved progress value"
  modulations:
    AttackTimeModulation:
      desc: "Multiplies the attack time per voice at note-on"
      scope: "per-voice"
    AttackLevelModulation:
      desc: "Multiplies the attack and hold level per voice at note-on"
      scope: "per-voice"
    DecayTimeModulation:
      desc: "Multiplies the decay time per voice at note-on"
      scope: "per-voice"
    SustainLevelModulation:
      desc: "Multiplies the sustain and decay target level (supports time-variant modulation)"
      scope: "per-voice"
    ReleaseTimeModulation:
      desc: "Multiplies the release time per voice at note-on"
      scope: "per-voice"
---

```
// Flex AHDSR Envelope - per-voice envelope with power-curve shaping
// noteOn/noteOff in -> modulation value (0-1) out

onNoteOn() {
    attackTime  = Attack  * AttackTimeModulation
    peakLevel   = AttackLevel * AttackLevelModulation
    decayTime   = Decay   * DecayTimeModulation
    sustainLevel = Sustain * SustainLevelModulation
    releaseTime = Release * ReleaseTimeModulation

    // Attack: ramp from 0 to peakLevel
    progress = powerCurve(counter / attackTime, AttackCurve)
    value = interpolate(0, peakLevel, progress)

    // Hold: stay at peakLevel (no curve, flat output)
    value = peakLevel    // for Hold duration

    // Decay: ramp from peakLevel to sustainLevel
    progress = powerCurve(counter / decayTime, DecayCurve)
    value = interpolate(peakLevel, sustainLevel, progress)

    // Sustain behaviour depends on Mode
    if (Mode == Note)
        value = sustainLevel    // hold until note-off
    if (Mode == Trigger)
        // skip sustain, advance to release
    if (Mode == Loop)
        // skip sustain, advance to release
}

onNoteOff() {
    // Release: ramp from current level to 0
    progress = powerCurve(counter / releaseTime, ReleaseCurve)
    value = interpolate(currentLevel, 0, progress)

    if (Mode == Loop && gateActive)
        // restart from attack
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Envelope Timing
    params:
      - { name: Attack, desc: "Time to ramp from zero to the attack level after note-on", range: "0 - 30000 ms", default: "5 ms" }
      - { name: Hold, desc: "Time held at the attack level before decay begins. Set to 0 to skip.", range: "0 - 30000 ms", default: "0 ms" }
      - { name: Decay, desc: "Time to fall from the attack level to the sustain level", range: "0 - 30000 ms", default: "100 ms" }
      - { name: Release, desc: "Time to fall from the current level to zero after note-off (or after decay in Trigger/Loop mode)", range: "0 - 30000 ms", default: "300 ms" }
  - label: Envelope Levels
    params:
      - name: AttackLevel
        desc: "Peak level reached at the end of the attack stage. Linear amplitude, not dB."
        range: "0.0 - 1.0"
        default: "1.0"
        hints:
          - type: warning
            text: "Clamped to be at least equal to the Sustain level. Lowering AttackLevel below Sustain raises it to match; raising Sustain above AttackLevel pushes AttackLevel up."
      - { name: Sustain, desc: "Level maintained while the note is held (in Note mode). Linear amplitude, not dB. A value of 0.5 corresponds to -6 dB.", range: "0.0 - 1.0", default: "0.5" }
  - label: Curve Shape
    params:
      - name: AttackCurve
        desc: "Curvature of the attack ramp. 0.5 is linear. Below 0.5 produces convex shapes (fast start); above 0.5 produces concave shapes (slow start)."
        range: "0.0 - 1.0"
        default: "0.5"
        hints:
          - type: info
            text: "Curve value changes are smoothed over 20 ms to prevent clicks during realtime adjustment. The same applies to DecayCurve and ReleaseCurve."
      - { name: DecayCurve, desc: "Curvature of the decay ramp. 0.5 is linear. Below 0.5 produces convex shapes; above 0.5 produces concave shapes.", range: "0.0 - 1.0", default: "0.5" }
      - { name: ReleaseCurve, desc: "Curvature of the release ramp. 0.5 is linear. Below 0.5 produces convex shapes; above 0.5 produces concave shapes.", range: "0.0 - 1.0", default: "0.5" }
  - label: Playback Control
    params:
      - { name: Mode, desc: "Envelope playback behaviour. Trigger: one-shot (skips sustain). Note: standard AHDSR with sustain hold. Loop: repeats attack-through-release while the note is held.", range: "Trigger, Note, Loop", default: "Note" }
      - { name: Monophonic, desc: "Forces all notes to share a single voice state. New notes either retrigger or continue the envelope depending on the Retrigger setting.", range: "Off / On", default: "(dynamic)" }
      - { name: Retrigger, desc: "Restarts the envelope when a new note arrives in monophonic mode. When off, the envelope continues from its current position (legato).", range: "Off / On", default: "On" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: AttackTimeModulation, desc: "Multiplies the attack time per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: AttackLevelModulation, desc: "Multiplies the attack and hold peak level per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: DecayTimeModulation, desc: "Multiplies the decay time per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: SustainLevelModulation, desc: "Multiplies the sustain level and decay target level. Accepts both voice-start and time-variant modulators.", scope: "per-voice", constrainer: "None" }
  - { name: ReleaseTimeModulation, desc: "Multiplies the release time per voice at note-on", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::

### Playback Modes

The three modes differ only in how the sustain and done states are handled:

- **Note:** The envelope holds at the sustain level until note-off triggers the release stage. This is standard AHDSR behaviour.
- **Trigger:** The envelope plays through attack, hold, decay, and release as a continuous one-shot. The sustain stage is skipped entirely. Note-off is ignored for stage progression (though it can still trigger release if it arrives during an earlier stage).
- **Loop:** Like Trigger, the sustain stage is skipped. When the release stage completes, the envelope restarts from the attack stage as long as the note is still held. On note-off, the current cycle completes and the voice ends.

### Sustain Modulation

The SustainLevelModulation chain is the only chain that accepts time-variant modulators (e.g. LFOs). All other chains are constrained to voice-start modulators. When a time-variant modulator is present on the sustain chain, the envelope switches to per-sample processing for that chain, which adds a small amount of CPU overhead.

### Visualisation with FlexAHDSRGraph

The envelope shape can be displayed and edited interactively using the FlexAHDSRGraph FloatingTile. This component draws the envelope path from parameter values and shows a playback ball tracking the last triggered voice. The FloatingTile has a `UseOneDimensionDrag` property (default: true) that constrains drag gestures to one axis at a time. Set it to false to allow simultaneous time and level adjustment in a single drag.

> [!Tip:Use a dedicated MIDI processor for realtime parameter control] When scripting velocity-to-parameter mappings for the FlexAHDSR, place the logic in a separate MIDI processor script rather than the Interface script. The Interface script runs deferred, so realtime parameter changes from note events require a non-deferred processor.

> [!Warning:Call addModuleStateToUserPreset for preset recall] The FlexAHDSR envelope state is not included in user presets by default. Call `Engine.addModuleStateToUserPreset("envelopeId")` to ensure the envelope configuration is saved and restored with presets.

### Look and Feel Callbacks

The FlexAHDSRGraph supports seven LAF callbacks for full visual customisation:

- `drawFlexAhdsrBackground` - background fill
- `drawFlexAhdsrFullPath` - complete envelope path
- `drawFlexAhdsrSegment` - individual stage segment (the `obj.active` property reflects whether the segment is currently being played, not hovered)
- `drawFlexAhdsrCurvePoint` - curve drag handle
- `drawFlexAhdsrDragPoint` - stage time/level drag handle
- `drawFlexAhdsrPosition` - playback ball position
- `drawFlexAhdsrText` - parameter label text

**See also:** $MODULES.AHDSR$ -- simpler AHDSR with coefficient-based curves and a single shared decay/release shape, $MODULES.SimpleEnvelope$ -- lighter-weight envelope with fewer stages and no curve control, $MODULES.TableEnvelope$ -- table-driven envelope for arbitrary shapes that cannot be expressed as power curves, $SN.envelope.flex_ahdsr$ -- scriptnode equivalent with per-segment curve control and trigger/loop modes
