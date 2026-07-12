---
title: MPE Modulator
moduleId: MPEModulator
type: Modulator
subtype: EnvelopeModulator
tags: [input]
builderPath: b.Modulators.MPEModulator
screenshot: /images/v2/reference/audio-modules/mpemodulator.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: PitchWheel, type: alternative, reason: "Monophonic pitch bend modulator - use when MPE per-voice glide is not needed" }
  - { id: MidiController, type: alternative, reason: "Monophonic CC modulator - use when MPE per-voice slide/pressure is not needed" }
commonMistakes:
  - title: "MPE panel required for activation"
    wrong: "Adding an MPE Modulator to the module tree and expecting it to work immediately"
    right: "Include an MPE panel widget in the interface so the end user can enable MPE mode"
    explanation: "MPE modulators start bypassed and only activate when global MPE mode is enabled through the MPE panel. Without the panel, the modulators never receive gesture data."
  - title: "Stroke and Lift are one-shot, not continuous"
    wrong: "Using Stroke mode and expecting continuous velocity tracking during the note"
    right: "Use Press, Slide, or Glide for continuous per-voice modulation during a note"
    explanation: "Stroke reads the velocity at note-on and Lift reads the velocity at note-off. Neither updates continuously. For ongoing gesture control, use one of the three continuous gesture types."
  - title: "SmoothedIntensity controls modulation depth, not smoothing"
    wrong: "Adjusting SmoothedIntensity to change the smoothing amount"
    right: "Use SmoothingTime to control smoothing speed; SmoothedIntensity sets overall modulation depth"
    explanation: "Despite the name, SmoothedIntensity is the modulator intensity (depth/amplitude). Setting it to zero disables the modulation output entirely."
forumReferences:
  - id: 1
    title: "MPE modulators are disabled until the MPE panel activates them"
    summary: "MPE modulators start disabled and only activate when the end user enables MPE through the MPE panel widget; instruments without a panel will never have their MPE modulators activated."
    topic: 1245
  - id: 2
    title: "Default MPE to off to avoid release trigger issues"
    summary: "Enabling MPE by default in presets can interfere with the release trigger mechanism; setting MPE off by default and letting the user opt in reduces this breakage."
    topic: 11871
  - id: 3
    title: "No scripting API to read or set individual MPE modulator values"
    summary: "There is no scripting API to check or set the current value of an MPE modulator; the only related call is Engine.isMpeEnabled() for global MPE state."
    topic: 6132
customEquivalent:
  approach: scriptnode
  moduleType: ScriptNode
  complexity: complex
  description: "MPE per-voice channel routing is deeply integrated into the voice management system. Replicating it requires matching incoming MIDI channels to active voices, which is not straightforward in scriptnode or HISEScript."
llmRef: |
  MPE Modulator (Modulator/EnvelopeModulator)

  Converts MPE gesture data into per-voice modulation signals. Five gesture types: Press (channel aftertouch), Slide (CC#74), Glide (pitch bend per channel), Stroke (note-on velocity), Lift (note-off velocity). Press, Slide, and Glide are continuous; Stroke and Lift fire once.

  Signal flow:
    MIDI event -> gesture type filter -> normalise to 0-1 -> table lookup -> one-pole smoother -> intensity scaling -> modulation out

  CPU: low, polyphonic (per-voice per-sample smoothing, scales with voice count)

  Parameters:
    Monophonic (Off/On, default dynamic) - shares a single voice state; aggregates max value across all channels
    Retrigger (Off/On, default On) - resets smoother on new notes in monophonic mode
    GestureCC (Press/Slide/Glide/Stroke/Lift, default dynamic) - selects which MPE gesture type to track
    SmoothingTime (0-2000 ms, default 200 ms) - one-pole smoothing time constant for gesture values
    DefaultValue (0-100%, default dynamic) - initial value when voice starts and no gesture data is available
    SmoothedIntensity (0-100%, default 100%) - modulation depth/amplitude (not a smoothing control)

  When to use:
    Per-voice expressive control from MPE controllers. Map pressure to filter cutoff, slide to timbre, glide to pitch. Requires an MPE panel in the interface for activation.

  Common mistakes:
    MPE modulators start bypassed - an MPE panel widget must be in the interface for them to activate.
    Stroke/Lift are one-shot (note-on/off only), not continuous like Press/Slide/Glide.
    SmoothedIntensity is the modulation depth, not a smoothing control.

  Custom equivalent:
    complex - MPE per-voice channel routing is deeply integrated into the voice system and not straightforward to replicate.

  See also:
    alternative PitchWheel - monophonic pitch bend modulator
    alternative MidiController - monophonic CC modulator
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
---
::

![MPE Modulator screenshot](/images/v2/reference/audio-modules/mpemodulator.png)

The MPE Modulator converts MIDI Polyphonic Expression (MPE) gesture data into per-voice modulation signals. Each voice is matched to a specific MIDI channel captured at note-on, so continuous gesture messages from an MPE controller reach only the correct voice. Five gesture types are available: three continuous types (Press, Slide, Glide) that update throughout the note, and two one-shot types (Stroke, Lift) that read velocity at note-on or note-off.

A lookup table shapes the normalised gesture value through a user-defined curve before a per-voice one-pole smoother removes any zipper noise. The table is always active - there is no toggle to bypass it. A fresh modulator uses a linear identity curve that passes values through unchanged.

> [!Warning:MPE panel required in the interface] MPE modulators start bypassed and only activate when global MPE mode is enabled. Include an MPE panel widget in your interface so the end user can switch MPE on. Without it, these modulators produce no output. [1]($FORUM_REF.1245$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    GestureCC:
      desc: "Selects which MPE gesture type to track"
      range: "Press, Slide, Glide, Stroke, Lift"
      default: "(dynamic)"
    Monophonic:
      desc: "Shares a single voice state and aggregates the maximum value across all channels"
      range: "Off / On"
      default: "(dynamic)"
    SmoothingTime:
      desc: "One-pole smoothing time constant for gesture values"
      range: "0 - 2000 ms"
      default: "200 ms"
    DefaultValue:
      desc: "Initial value when the voice starts and no gesture data is available"
      range: "0 - 100%"
      default: "(dynamic)"
    SmoothedIntensity:
      desc: "Modulation depth applied to the final output"
      range: "0 - 100%"
      default: "100%"
  functions:
    tableLookup:
      desc: "Maps the normalised gesture value through a user-defined curve (always active)"
    smoothToTarget:
      desc: "One-pole IIR smoother that interpolates toward the target value per sample"
  modulations: {}
---

```
// MPE Modulator - per-voice gesture to modulation
// MIDI in -> modulation out (per voice)

onNoteOn() {
    channel = note.channel         // stored for per-voice routing
    value = DefaultValue

    if (GestureCC == Stroke)
        value = tableLookup(note.velocity)

    smoother.set(value)
}

onGesture(channel, gestureValue) {
    // continuous: Press, Slide, or Glide
    value = normalise(gestureValue)    // 0.0 - 1.0

    if (Monophonic)
        value = maxAcrossChannels(value)

    value = tableLookup(value)
    smoother.setTarget(value)          // matched by channel
}

perSample() {
    output = smoothToTarget(SmoothingTime)
    output = output * SmoothedIntensity
    return output
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Gesture
    params:
      - name: GestureCC
        desc: "Selects which MPE gesture type this modulator responds to. Press, Slide, and Glide provide continuous control during a note; Stroke and Lift read velocity once at note-on or note-off."
        range: "Press, Slide, Glide, Stroke, Lift"
        default: "(dynamic)"
        hints:
          - type: info
            text: "Changing the gesture type resets all active voice states. Slide mode also changes the table editor display to show pitch-bend range on the X-axis."
  - label: Smoothing
    params:
      - name: SmoothingTime
        desc: "Time constant for the per-voice one-pole smoother. Higher values produce slower, more gradual transitions between gesture values."
        range: "0 - 2000 ms"
        default: "200 ms"
      - name: DefaultValue
        desc: "Starting value for each voice when no gesture data has been received yet. The display units depend on the modulation mode (gain, pitch, or pan)."
        range: "0 - 100%"
        default: "(dynamic)"
      - name: SmoothedIntensity
        desc: "Overall modulation depth. Controls how strongly the gesture data affects the target parameter. Despite the name, this is not related to smoothing."
        range: "0 - 100%"
        default: "100%"
        hints:
          - type: warning
            text: "This parameter controls modulation depth, not smoothing. Setting it to zero disables the modulation output entirely."
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "When enabled, all voices share a single state and gesture values are aggregated by taking the maximum across all active channels", range: "Off / On", default: "(dynamic)" }
      - name: Retrigger
        desc: "In monophonic mode, resets the smoother to the start value when a new note is triggered while one is already playing"
        range: "Off / On"
        default: "On"
        hints:
          - type: info
            text: "Only has an effect when Monophonic is enabled. In polyphonic mode this parameter is ignored."
---
::

### Gesture Types

The five gesture types map to standard MIDI and MPE message types:

| Gesture | MIDI source | Behaviour | Value range |
|---------|-------------|-----------|-------------|
| **Press** | Channel aftertouch | Continuous per-voice pressure | 0.0 - 1.0 |
| **Slide** | CC #74 (timbre) | Continuous per-voice timbre | 0.0 - 1.0 |
| **Glide** | Pitch wheel (per channel) | Continuous per-voice pitch bend | 0.0 - 1.0 (0.5 = centre) |
| **Stroke** | Note-on velocity | One-shot at note-on | 0.0 - 1.0 |
| **Lift** | Note-off velocity | One-shot at note-off | 0.0 - 1.0 |

Press, Slide, and Glide update continuously throughout the note and are the primary MPE dimensions. Stroke and Lift behave like voice-start or voice-stop modulators - they capture a single value and do not change afterwards.

### MPE Setup

MPE modulators are designed to be activated by the end user through a dedicated MPE panel widget in the interface. The workflow is:

1. Add one or more MPE Modulators to modulation chains in the module tree (e.g. gain, filter cutoff, pitch).
2. Place an MPE panel widget in the interface design.
3. The end user enables MPE via the panel at runtime, which activates all MPE modulators simultaneously.

There is no scripting API for enabling individual MPE modulators or reading their current values. The only related API call is `Engine.isMpeEnabled()`, which checks the global MPE state. [3]($FORUM_REF.6132$)

> [!Tip:Default MPE to off in presets] When building presets, leave MPE disabled by default. Enabling it unconditionally can interfere with release triggers and other voice management features. Let the end user opt in through the MPE panel. [2]($FORUM_REF.11871$)

### Per-Voice Channel Routing

In polyphonic mode, each voice captures the MIDI channel from its note-on event. When continuous gesture messages arrive, only the voice whose stored channel matches the incoming message channel receives the update. This implements the MPE one-channel-per-voice model.

In monophonic mode, channel matching is bypassed. All gesture data feeds into a single shared state. For Press and Slide, the maximum value across all active channels is used. For Glide, the channel with the greatest distance from centre (0.5) is selected.

### Scripting Access

To access the table programmatically, use `Synth.getTableProcessor()` with the modulator name. To control the modulator as a modulation source (e.g. bypassing it), use `Synth.getModulator()`. Both references are needed to use the full interface.

**See also:** $MODULES.PitchWheel$ -- monophonic pitch bend modulator for non-MPE setups, $MODULES.MidiController$ -- monophonic CC modulator for non-MPE setups
