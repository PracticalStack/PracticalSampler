---
title: Simple Envelope
moduleId: SimpleEnvelope
type: Modulator
subtype: EnvelopeModulator
tags: [generator]
builderPath: b.Modulators.SimpleEnvelope
screenshot: /images/v2/reference/audio-modules/simpleenvelope.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: AHDSR, type: upgrade, reason: "Full five-stage envelope with adjustable sustain level and hold stage" }
  - { id: FlexAHDSR, type: upgrade, reason: "Draggable envelope curves with multiple shape modes per stage" }
  - { id: TableEnvelope, type: alternative, reason: "Table-driven envelope shape for arbitrary attack curves" }
  - { id: "envelope.simple_ar", type: scriptnode, reason: "Scriptnode equivalent -- two-stage attack/release envelope" }
commonMistakes:
  - title: "No sustain level parameter"
    wrong: "Looking for a sustain knob to control the held level"
    right: "The sustain level is always 1.0 (full output) - use AHDSR if you need an adjustable sustain"
    explanation: "SimpleEnvelope is a pure attack-release envelope. The held level between attack and release is fixed at maximum."
  - title: "Release change affects all releasing voices"
    wrong: "Automating the Release parameter expecting each voice to keep its original release time"
    right: "All voices share the same release coefficient - changing Release mid-playback alters every currently releasing voice"
    explanation: "Unlike the attack time (which is captured per voice at note-on), the release coefficient is global. Adjusting it reshapes all active release tails simultaneously."
  - title: "Release not triggering with artificial notes"
    wrong: "Using Synth.playNote() without sending a matching note-off"
    right: "Store the event ID returned by Synth.playNote() and call Synth.noteOffByEventId() to trigger the release stage"
    explanation: "Artificial notes need an explicit note-off event. Without it, the envelope stays in sustain indefinitely."
customEquivalent:
  approach: hisescript
  moduleType: ScriptEnvelopeModulator
  complexity: trivial
  description: "Implement attack ramp and release ramp in onVoiceStart/onVoiceStop callbacks"
llmRef: |
  Simple Envelope (Modulator/EnvelopeModulator)

  A lightweight two-stage (attack + release) envelope with an implicit sustain at full level (1.0). Lighter than AHDSR due to SIMD-optimised sustain/idle states and a simpler state machine.

  Signal flow:
    noteOn -> [attack mod chain] -> attack ramp (0 to 1) -> sustain (hold at 1.0) -> noteOff -> release ramp (1 to 0) -> idle

  CPU: low, polyphonic (SIMD fast-path during sustain and idle)

  Parameters:
    Attack (0-20000 ms, default 5 ms) - attack ramp time, modulatable per voice via the Attack Time Modulation chain
    Release (0-20000 ms, default 10 ms) - release ramp time, shared across all voices (not modulatable)
    LinearMode (Off/On, default On) - On: straight-line ramps; Off: exponential curves (steeper initial release, longer tail)
    Monophonic (Off/On, default dynamic) - all voices share a single envelope state
    Retrigger (Off/On, default On) - in monophonic mode, new notes restart the envelope from the current value

  Modulation chains:
    AttackTimeModulation - scales the attack time per voice at note-on (VoiceStartModulator only)

  When to use:
    Volume shaping where only attack and release matter. Ideal for pads, risers, or any patch where sustain is always at full level. Use AHDSR when you need adjustable sustain or decay stages.

  Common mistakes:
    Sustain is always 1.0 - there is no sustain level parameter.
    Changing Release affects all currently releasing voices (shared coefficient).
    Artificial notes from Synth.playNote() need Synth.noteOffByEventId() to trigger release.
    No loop stage - use scriptnode for looping envelopes.

  Custom equivalent:
    hisescript via ScriptEnvelopeModulator: implement attack/release ramps in onVoiceStart/onVoiceStop.

  See also:
    upgrade AHDSR - full five-stage envelope with adjustable sustain
    upgrade FlexAHDSR - draggable curves with multiple shape modes
    alternative TableEnvelope - table-driven shapes for arbitrary curves
    [scriptnode] envelope.simple_ar -- scriptnode equivalent -- two-stage attack/release envelope
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![Simple Envelope screenshot](/images/v2/reference/audio-modules/simpleenvelope.png)

The Simple Envelope is a lightweight two-stage envelope modulator with attack and release. The sustain level is fixed at 1.0 (full output) - there is no decay, adjustable sustain, or loop stage. The sustain and idle states skip per-sample processing entirely, making this the most CPU-efficient envelope in HISE at high voice counts. For looping envelope behaviour, use a scriptnode network instead.

Both stages support linear or exponential curves via the **LinearMode** toggle. Linear mode produces straight-line ramps. Exponential mode produces a curved attack (moderate curvature) and a classic exponential release with a steep initial drop followed by a long tail, cutting off at approximately -80 dB. The attack time can be modulated per voice through the Attack Time Modulation chain; the release time is shared across all voices.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Base attack time - ramp duration from silence to full level"
      range: "0 - 20000 ms"
      default: "5 ms"
    Release:
      desc: "Release time - ramp duration from current level to silence (shared across all voices)"
      range: "0 - 20000 ms"
      default: "10 ms"
    LinearMode:
      desc: "Toggles between straight-line ramps (On) and exponential curves (Off)"
      range: "Off / On"
      default: "On"
  functions:
    calcAttackCoefficients:
      desc: "Computes per-voice ramp coefficients from the modulated attack time"
    exponentialCurve:
      desc: "One-pole filter formula producing a curved ramp instead of a straight line"
  modulations:
    AttackTimeModulation:
      desc: "Scales the attack time per voice at note-on"
      scope: "per-voice"
---

```
// Simple Envelope - two-stage AR envelope
// noteOn/noteOff in -> modulation value (0-1) out

onNoteOn() {
    attackTime = Attack * AttackTimeModulation
    calcAttackCoefficients(attackTime)
    state = ATTACK
}

perSample() {
    if (state == ATTACK)
        if (LinearMode)
            value += delta             // straight ramp up
        else
            value = exponentialCurve(value)  // curved ramp up

        if (value >= 1.0)
            state = SUSTAIN

    if (state == SUSTAIN)
        value = 1.0                    // held at full level

    if (state == RELEASE)
        if (LinearMode)
            value -= delta             // straight ramp down
        else
            value = exponentialCurve(value)  // curved ramp down

        if (value <= threshold)
            state = IDLE

    return value
}

onNoteOff() {
    state = RELEASE
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Envelope
    params:
      - { name: Attack, desc: "Ramp time from silence to full level. Modulatable per voice through the Attack Time Modulation chain.", range: "0 - 20000 ms", default: "5 ms" }
      - name: Release
        desc: "Ramp time from current level to silence."
        range: "0 - 20000 ms"
        default: "10 ms"
        hints:
          - type: warning
            text: "The release coefficient is shared across all voices. Changing this parameter instantly affects all currently releasing voices, not just new ones."
      - name: LinearMode
        desc: "Selects the curve shape. On: straight-line ramps. Off: exponential curves with a steeper initial release and longer tail."
        range: "Off / On"
        default: "On"
        hints:
          - type: info
            text: "In exponential mode, the release tail is cut off at approximately -80 dB rather than ramping to true zero. This is inaudible but frees the voice marginally earlier than linear mode."
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "When enabled, all voices share a single envelope state. The release stage only begins when all held keys are released.", range: "Off / On", default: "(dynamic)" }
      - { name: Retrigger, desc: "In monophonic mode, new notes restart the envelope from the current value rather than continuing (legato). Has no effect when Monophonic is off.", range: "Off / On", default: "On" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: AttackTimeModulation, desc: "Scales the attack time per voice. Evaluated once at note-on.", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::

### Artificial Notes and Release

> [!Tip:Store event IDs for artificial notes] When triggering notes with `Synth.playNote()`, store the returned event ID (e.g. in a [MIDIList]($API.MIDIList$)) and call `Synth.noteOffByEventId()` to ensure the release stage fires. Without an explicit note-off, the envelope remains in sustain indefinitely.

**See also:** $MODULES.AHDSR$ -- full five-stage envelope with adjustable sustain level and hold stage, $MODULES.FlexAHDSR$ -- draggable envelope curves with multiple shape modes per stage, $MODULES.TableEnvelope$ -- table-driven envelope shape for arbitrary attack curves, $SN.envelope.simple_ar$ -- scriptnode equivalent -- two-stage attack/release envelope
