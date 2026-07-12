---
title: Random Modulator
moduleId: Random
type: Modulator
subtype: VoiceStartModulator
tags: [generator]
builderPath: b.Modulators.Random
screenshot: /images/v2/reference/audio-modules/random.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: Velocity, type: alternative, reason: "Deterministic per-note value from MIDI velocity instead of a random value" }
  - { id: Constant, type: disambiguation, reason: "Outputs a fixed value rather than a random one" }
  - { id: ArrayModulator, type: alternative, reason: "Explicit per-note values from a 128-entry slider pack instead of random generation" }
commonMistakes:
  - title: "Output is unrelated to MIDI data"
    wrong: "Expecting the random value to correlate with velocity, note number, or any MIDI input"
    right: "The output is purely random - it ignores the incoming MIDI event entirely"
    explanation: "Unlike Velocity or KeyNumber, Random generates its own value with no MIDI input. Use those modules if you need event-dependent modulation."
  - title: "UseTable reshapes distribution, not range"
    wrong: "Using the table to extend the output beyond 0-1"
    right: "The table remaps the probability distribution within the 0-1 range"
    explanation: "Both input and output of the table are 0-1. Flat table regions concentrate probability there; steep regions spread it."
  - title: "Voices are independent"
    wrong: "Expecting simultaneous voices to share or correlate their random values"
    right: "Each voice receives an independent random value"
    explanation: "Every note-on generates a fresh random value. There is no correlation between voices triggered at the same time."
forumReferences:
  - id: 1
    title: "Sample start offset silently capped by preload buffer size"
    summary: "When using Random as a sample start modulator, the maximum usable offset is limited by the preload buffer size; a large offset with a small preload buffer is silently clamped."
    topic: 9526
  - id: 2
    title: "Pitch randomisation via Random modulator only works in instrument plugins"
    summary: "A Random modulator on the pitch parameter requires a voice chain; in an FX plugin there is no note chain, so pitch randomisation must be done via Message.setTransposeAmount() in script."
    topic: 2210
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Use Math.random() in the onVoiceStart callback"
llmRef: |
  Random Modulator (Modulator/VoiceStartModulator)

  Generates an independent random value (0-1) for each voice at note-on. An optional lookup table reshapes the probability distribution without changing the output range.

  Signal flow:
    noteOn -> random float [0, 1) -> [table lookup] -> modulation out

  CPU: negligible, polyphonic (runs once per voice on note-on)

  Parameters:
    UseTable (Off/On, default Off) - enables a lookup table that remaps the uniform random distribution into a custom probability curve

  When to use:
    Add controlled randomness to any modulatable parameter - sample start offset, pitch detune, gain variation, filter cutoff jitter. Enable UseTable to bias the distribution towards specific value ranges.

  Common mistakes:
    Output is purely random with no MIDI input - it does not read velocity or note number.
    UseTable reshapes the distribution within 0-1, it does not change the output range.
    Each voice gets an independent value - simultaneous voices are uncorrelated.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: use Math.random() in onVoiceStart.

  See also:
    alternative Velocity - deterministic per-note value from MIDI velocity
    disambiguation Constant - fixed modulation value, no randomness
    alternative ArrayModulator - explicit per-note values from a slider pack
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![Random Modulator screenshot](/images/v2/reference/audio-modules/random.png)

The Random Modulator generates an independent random value between 0 and 1 for each voice at note-on. It ignores all MIDI data - the output is purely random regardless of velocity, note number, or any other event property. Typical uses include sample start offset variation, subtle pitch detune, gain humanisation, and filter cutoff jitter. When using it as a sample start modulator, the usable range is bounded by the sampler's preload buffer size — a large offset value with a small preload buffer is silently clamped. [1]($FORUM_REF.9526$) Pitch randomisation via this modulator requires an instrument plugin with a voice chain; in an FX plugin context, use `Message.setTransposeAmount()` from script instead. [2]($FORUM_REF.2210$)

Enabling **UseTable** activates a curve editor that reshapes the probability distribution. The random value becomes the X position in the table and the Y value becomes the output. Flat regions in the table concentrate probability towards those output values, while steep regions spread it. With the table disabled (the default), the distribution is uniform.

## Signal Path

::signal-path
---
glossary:
  parameters:
    UseTable:
      desc: "Enables the lookup table for custom probability distribution shaping"
      range: "Off / On"
      default: "Off"
  functions:
    tableLookup:
      desc: "Maps the random value through a user-defined curve (0-1 input, 0-1 output)"
  modulations: {}
---

```
// Random Modulator - random value per voice
// noteOn in -> modulation out (per voice)

onNoteOn() {
    value = randomFloat(0.0, 1.0)

    if (UseTable)
        value = tableLookup(value)

    return value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Distribution
    params:
      - name: UseTable
        desc: "Enables a lookup table with a curve editor for reshaping the random distribution"
        range: "Off / On"
        default: "Off"
        hints:
          - type: info
            text: "The default table is a linear ramp (identity mapping), so enabling the table without editing the curve produces the same output as leaving it disabled."
---
::

### Scripting with Tables

The Random module implements the TableProcessor interface. To manipulate the table from script, obtain a reference with `Synth.getTableProcessor()`. If you also need access to the modulator API (e.g. `setAttribute`, `getCurrentLevel`), create a second reference with `Synth.getModulator()` - both point to the same module but expose different APIs.

```javascript
const var randomTable = Synth.getTableProcessor("Random Modulator");
const var randomMod   = Synth.getModulator("Random Modulator");

// Enable the table via the modulator reference
randomMod.setAttribute(randomMod.UseTable, 1);

// Manipulate the curve via the table processor reference
randomTable.setTablePoint(0, 0, 0.0, 0.0, 0.5);
```

**See also:** $MODULES.Velocity$ -- deterministic per-note value from MIDI velocity, $MODULES.Constant$ -- outputs a fixed modulation value with no randomness, $MODULES.ArrayModulator$ -- explicit per-note values from a 128-entry slider pack
