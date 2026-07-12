---
title: Table Envelope
moduleId: TableEnvelope
type: Modulator
subtype: EnvelopeModulator
tags: [generator]
builderPath: b.Modulators.TableEnvelope
screenshot: /images/v2/reference/audio-modules/tableenvelope.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: AHDSR, type: alternative, reason: "Fixed-stage envelope with curve parameters instead of drawable tables" }
  - { id: FlexAHDSR, type: alternative, reason: "Multi-stage envelope with draggable curve points and multiple release modes" }
  - { id: SimpleEnvelope, type: alternative, reason: "Two-parameter envelope with no table editor" }
commonMistakes:
  - title: "Attack and release use separate tables"
    wrong: "Expecting a single continuous table shape from attack through release"
    right: "The attack table and release table are independent - each defines its own shape over its own time range"
    explanation: "The attack table is swept over the Attack time, then the release table is swept over the Release time. They are not two halves of one table."
  - title: "Release output scales from the level at note-off"
    wrong: "Assuming the release table output is an absolute gain value"
    right: "The release table output is multiplied by the envelope level at the moment of note-off"
    explanation: "If the attack phase was interrupted early (e.g. the envelope was at 0.6 when the key was released), the release starts from 0.6, not 1.0."
  - title: "Table Y-axis is always 0-1"
    wrong: "Drawing the table to set absolute gain or dB values"
    right: "Table values are normalised multipliers in the 0-1 range; the timing is set separately via the Attack and Release parameters"
    explanation: "The table defines the shape of the curve. The Attack or Release parameter controls how long it takes to sweep through that shape."
  - title: "Minimum parameter value is 1 ms"
    wrong: "Setting Attack or Release to 0 ms via setAttribute in script"
    right: "Clamp values with Math.max(1, value) before calling setAttribute"
    explanation: "The UI enforces a 1 ms minimum, but script can bypass this and cause undefined behaviour."
forumReferences:
  - id: 1
    title: "Scriptnode workaround for guaranteed attack playthrough"
    summary: "The stock TableEnvelope stops the attack table on note-off; to ensure the attack always plays to completion, build a scriptnode network with a ramp driven by note-on (not gate) feeding through a table node."
    topic: 9922
customEquivalent:
  approach: hisescript
  moduleType: ScriptEnvelopeModulator
  complexity: simple
  description: "Use Synth.getTableProcessor() to access the lookup tables and implement timed sweeps in script callbacks"
llmRef: |
  Table Envelope (Modulator/EnvelopeModulator)

  A polyphonic envelope modulator with two user-drawn lookup tables: one for the attack shape and one for the release shape. Each table is swept left-to-right over its respective time parameter.

  Signal flow:
    noteOn -> evaluate mod chains -> sweep attack table over Attack time -> hold sustain (last attack value) -> noteOff -> capture level -> sweep release table * captured level over Release time -> idle

  CPU: low, polyphonic (per-sample table lookup with linear interpolation per voice)

  Parameters:
    Monophonic (Off/On, default dynamic) - enables single-voice mode
    Retrigger (Off/On, default On) - restarts the envelope on legato notes in monophonic mode with a short glide
    Attack (1 - 20000 ms, default 20 ms) - time to sweep through the attack table
    Release (1 - 20000 ms, default 20 ms) - time to sweep through the release table

  Modulation chains:
    AttackTimeModulation (per-voice, VoiceStartModulator) - scales the attack time
    ReleaseTimeModulation (per-voice, VoiceStartModulator) - scales the release time

  Key behaviours:
    Two independent tables (attack and release), each with 512-entry resolution and linear interpolation.
    Sustain level equals the rightmost value of the attack table.
    Release output is multiplied by the envelope level at the moment of note-off.
    If the attack table ends at or below 0.01, the envelope skips sustain and enters release immediately (one-shot mode, polyphonic only).
    Retrigger glide takes approximately 4.5 ms at 44.1 kHz (fixed rate, not configurable).
    Table graph points cannot be modified in realtime.

  When to use:
    When you need full control over the attack and release curve shapes beyond what fixed-stage envelopes offer. Particularly useful for percussive one-shot shapes, custom fade-ins, or non-standard release curves.

  Common mistakes:
    Attack and release are separate tables, not one continuous shape.
    Release output is scaled by the level at note-off, not used as an absolute value.
    Attack/Release minimum is 1 ms - setting 0 via script causes issues.

  Custom equivalent:
    hisescript via ScriptEnvelopeModulator: use Synth.getTableProcessor() to access lookup tables.

  See also:
    alternative AHDSR - fixed AHDSR stages with exponential curve parameters
    alternative FlexAHDSR - draggable multi-stage curves with release modes
    alternative SimpleEnvelope - simpler two-parameter envelope
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![Table Envelope screenshot](/images/v2/reference/audio-modules/tableenvelope.png)

The Table Envelope is a polyphonic envelope modulator with two independent lookup tables that define the attack and release shapes. Unlike fixed-stage envelopes such as the [AHDSR]($MODULES.AHDSR$), the curve shape is fully user-defined - you draw the attack shape in one table and the release shape in another.

Each table contains 512 entries and is swept left-to-right over its respective time parameter. The **attack table** defines how the envelope rises after a note-on. When the attack completes, the envelope holds at the value found at the right edge of the attack table (the sustain level). On note-off, the envelope captures its current level and sweeps through the **release table**, multiplying the table output by that captured level. This means early note-off (before the attack finishes) produces a proportionally quieter release. Both tables output values in the 0-1 range - they define the curve shape, while the Attack and Release parameters control the timing.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Time to sweep through the attack table from left to right"
      range: "1 - 20000 ms"
      default: "20 ms"
    Release:
      desc: "Time to sweep through the release table from left to right"
      range: "1 - 20000 ms"
      default: "20 ms"
    Monophonic:
      desc: "Enables single-voice mode where all notes share one envelope state"
      range: "Off / On"
      default: "(dynamic)"
    Retrigger:
      desc: "Restarts the envelope on new notes in monophonic mode with a short glide"
      range: "Off / On"
      default: "On"
  functions:
    attackTableLookup:
      desc: "Reads the attack table at the current normalised position (0-1 input, 0-1 output)"
    releaseTableLookup:
      desc: "Reads the release table at the current normalised position (0-1 input, 0-1 output)"
  modulations:
    AttackTimeModulation:
      desc: "Scales the attack time per voice (evaluated once at note-on)"
      scope: "per-voice"
    ReleaseTimeModulation:
      desc: "Scales the release time per voice (evaluated once at note-on)"
      scope: "per-voice"
---

```
// Table Envelope - two-table attack/release envelope
// noteOn/noteOff in -> modulation out (per voice, per sample)

onNoteOn() {
    attackTime = Attack * AttackTimeModulation
    releaseTime = Release * ReleaseTimeModulation

    if (attackTime ~= 0)
        value = 1.0              // skip to sustain
    else
        // ATTACK: sweep attack table left-to-right
        value = attackTableLookup(position / attackTime)

    // SUSTAIN: hold last attack table value until note-off

    if (Monophonic && Retrigger)
        // glide from current value to attack table start (~4.5 ms)
}

onNoteOff() {
    releaseGain = value          // capture level at note-off

    // RELEASE: sweep release table left-to-right
    value = releaseGain * releaseTableLookup(position / releaseTime)

    // when release completes -> value = 0, voice idle
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Envelope Timing
    params:
      - name: Attack
        desc: "Time to sweep through the attack table after note-on. The table shape is stretched linearly over this duration."
        range: "1 - 20000 ms"
        default: "20 ms"
        hints:
          - type: warning
            text: "If a note is released before the attack completes, the table freezes at its current position. The captured level becomes the starting point for the release. To guarantee the attack always plays to completion, use a scriptnode network: drive a ramp from note-on (not gate) through a table node for the attack shape, then feed a separate table for release. [1]($FORUM_REF.9922$)"
          - type: warning
            text: "Modulating the attack time below 1 ms can cause silence. Ensure velocity or random modulators on the AttackTimeModulation chain cannot push the effective time below 1 ms."
      - { name: Release, desc: "Time to sweep through the release table after note-off. The release output is multiplied by the level at the moment of note-off.", range: "1 - 20000 ms", default: "20 ms" }
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "Enables monophonic mode where all notes share a single envelope state. The voice is never reclaimed while active.", range: "Off / On", default: "(dynamic)" }
      - { name: Retrigger, desc: "When monophonic mode is active, restarts the envelope on new legato notes. A short glide (~4.5 ms) smooths the transition to the start of the attack table.", range: "Off / On", default: "On" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: AttackTimeModulation, desc: "Scales the attack time. Evaluated once at note-on. Higher values produce longer attack times.", scope: "per-voice", constrainer: "VoiceStartModulator" }
  - { name: ReleaseTimeModulation, desc: "Scales the release time. Evaluated once at note-on. Higher values produce longer release times.", scope: "per-voice", constrainer: "VoiceStartModulator" }
---
::

### One-Shot Mode

If the attack table ends at or below approximately zero (0.01), the envelope skips sustain entirely and enters the release phase immediately. This allows the Table Envelope to function as a one-shot shape for percussive sounds - draw a transient in the attack table that returns to zero, and the voice will release automatically. This behaviour applies in polyphonic mode only.

### Scripting with Tables

Table graph points cannot be modified in realtime. Each change triggers a full recalculation of the lookup table, so per-sample or per-block table updates from script are not safe. To link multiple Table Envelope modules so they share the same curve, use `exportAsBase64()` and `restoreFromBase64()` with a Broadcaster that propagates changes when the table is edited.

> [!Tip:Read table values from script] Use `getTableValue()` on a reference obtained via `Synth.getTableProcessor()` to read the curve shape at any normalised position (0-1). This is useful for driving custom scripted logic based on the table shape.

**See also:** $MODULES.AHDSR$ -- fixed-stage envelope with exponential curve parameters for standard ADSR shapes, $MODULES.FlexAHDSR$ -- multi-stage envelope with draggable curve points and multiple release modes, $MODULES.SimpleEnvelope$ -- two-parameter envelope when full table control is not needed
