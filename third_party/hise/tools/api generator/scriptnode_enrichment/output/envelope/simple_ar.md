---
title: Simple AR Envelope
description: "A lightweight attack-release envelope with curve shaping and a fixed sustain at full level."
factoryPath: envelope.simple_ar
factory: envelope
polyphonic: true
tags: [envelope, ar, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "envelope.ahdsr", type: alternative, reason: "Full AHDSR when hold, decay, or adjustable sustain are needed" }
  - { id: "envelope.voice_manager", type: companion, reason: "Connect the Gate output here to manage voice lifecycle" }
  - { id: "SimpleEnvelope", type: module, reason: "Module-tree equivalent -- two-stage attack/release envelope" }
commonMistakes:
  - title: "Sustain is always at full level"
    wrong: "Expecting an adjustable sustain level like the AHDSR envelopes."
    right: "simple_ar always sustains at 1.0. Use envelope.ahdsr or envelope.flex_ahdsr if you need a configurable sustain level."
    explanation: "This envelope has only two stages: attack (ramp to 1.0) and release (ramp to 0.0). There is no sustain parameter."
  - title: "Default AttackCurve differs from AHDSR"
    wrong: "Assuming AttackCurve defaults to 0.5 (linear) as in envelope.ahdsr."
    right: "The default AttackCurve is 0.0, which gives a pure exponential response."
    explanation: "Unlike the AHDSR envelope where 0.5 is the default (linear), simple_ar defaults to 0.0 (exponential). Adjust to 0.5 for linear attack behaviour."
llmRef: |
  envelope.simple_ar

  Lightweight attack-release envelope. Ramps to 1.0 on note-on, sustains at full level, ramps to 0.0 on note-off. Multiplies audio by envelope value and sends CV + Gate modulation outputs.

  Signal flow:
    MIDI note-on/off or Gate -> smoother -> envelope value
    audio in * envelope value -> audio out
    envelope value -> CV output (0..1)
    voice active -> Gate output (0 or 1)

  CPU: negligible, polyphonic

  Parameters:
    Attack (0-1000 ms, default 10), Release (0-1000 ms, default 10)
    Gate (Off/On, default Off; manual trigger)
    AttackCurve (0-1, default 0.0; 0=exponential, 0.5=linear, 1=power curve)

  When to use:
    Quick fade-in/fade-out for pads, risers, or any voice that needs a simple amplitude shape without configurable sustain. Lowest CPU of the envelope nodes.

  Common mistakes:
    - No sustain parameter; always sustains at 1.0
    - Default AttackCurve is 0.0 (exponential), not 0.5 (linear)

  See also:
    [alternative] envelope.ahdsr -- full AHDSR with configurable sustain
    [companion] envelope.voice_manager -- voice lifecycle from Gate output
    [module] SimpleEnvelope -- module-tree equivalent -- two-stage attack/release envelope
---

A lightweight two-stage envelope that ramps to full level on note-on and fades to zero on note-off. It multiplies the input audio by the envelope value and sends CV and Gate modulation outputs. The sustain level is fixed at 1.0 and cannot be adjusted.

This envelope has the lowest CPU cost of the envelope nodes, making it a good choice when you only need a fade-in and fade-out without the additional stages of a full AHDSR. The AttackCurve parameter blends between exponential, linear, and power curve shapes for both the attack and release segments. Values between the key points (0, 0.5, 1) interpolate smoothly between the curve types.

### Setup

The Gate output should be connected to an [envelope.voice_manager]($SN.envelope.voice_manager$) node for proper voice lifecycle management.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Attack:
      desc: "Rise time from zero to full level"
      range: "0 - 1000 ms"
      default: "10.0"
    Release:
      desc: "Fall time from current level toward zero"
      range: "0 - 1000 ms"
      default: "10.0"
    Gate:
      desc: "Manual gate trigger, independent of MIDI"
      range: "Off / On"
      default: "Off"
    AttackCurve:
      desc: "Curve shape: 0 = exponential, 0.5 = linear, 1 = power curve"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    smoother:
      desc: "Blends between exponential follower and linear ramp based on AttackCurve"
    multiply:
      desc: "Applies the envelope value to each audio sample"
---

```
// envelope.simple_ar - attack/release envelope
// audio + MIDI in -> audio out, CV out, Gate out

onNoteOn() {
    target = 1.0                        // always sustains at full level
}

process(input) {
    value = smoother(target)            // ramp toward target using Attack time
    output = input * multiply(value)
    cvOut = value
}

onNoteOff() {
    target = 0.0                        // ramp down using Release time
    if (value == 0.0)
        gateOut = 0                     // signal voice done
}

// AttackCurve controls the ramp shape:
//   0.0 = pure exponential
//   0.5 = pure linear
//   1.0 = power curve (slow start)
```

::

## Parameters

::parameter-table
---
groups:
  - label: Timing
    params:
      - { name: Attack, desc: "Rise time from zero to full level. Skewed toward lower values for fine control of short attacks.", range: "0 - 1000 ms", default: "10.0" }
      - { name: Release, desc: "Fall time from the current level toward zero after note-off.", range: "0 - 1000 ms", default: "10.0" }
  - label: Shape
    params:
      - { name: AttackCurve, desc: "Controls the ramp shape for both attack and release. 0 = exponential (default), 0.5 = linear, 1 = power curve (very slow start).", range: "0.0 - 1.0", default: "0.0" }
  - label: Control
    params:
      - { name: Gate, desc: "Manual gate trigger, independent of MIDI note events. Useful for modulation-driven triggering.", range: "Off / On", default: "Off" }
---
::

**See also:** $SN.envelope.ahdsr$ -- full AHDSR with configurable sustain, $SN.envelope.voice_manager$ -- voice lifecycle management, $MODULES.SimpleEnvelope$ -- module-tree equivalent -- two-stage attack/release envelope
