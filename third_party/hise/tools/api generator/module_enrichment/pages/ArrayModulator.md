---
title: Array Modulator
moduleId: ArrayModulator
type: Modulator
subtype: VoiceStartModulator
tags: [input]
builderPath: b.Modulators.ArrayModulator
screenshot: /images/v2/reference/audio-modules/arraymodulator.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: Velocity, type: alternative, reason: "Reads MIDI velocity instead of per-note slider values" }
  - { id: KeyNumber, type: alternative, reason: "Table-based note mapping with a continuous curve instead of per-note discrete values" }
  - { id: Constant, type: disambiguation, reason: "Also a VoiceStartModulator but outputs a single fixed value for all notes" }
commonMistakes:
  - title: "Default values are all 1.0"
    wrong: "Adding an Array Modulator and expecting it to shape the sound immediately"
    right: "Edit the slider pack to set per-note values - the default of 1.0 means full (transparent) modulation for every note"
    explanation: "All 128 sliders default to 1.0, which means the modulator has no audible effect until you lower individual slider values."
  - title: "Slider pack is fixed at 128 entries"
    wrong: "Trying to resize the slider pack to match a smaller key range"
    right: "The slider pack always has exactly 128 entries, one per MIDI note - adjust only the values you need"
    explanation: "The 128-entry size is fixed to match the MIDI note range (0-127) and cannot be changed."
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Use an array indexed by Message.getNoteNumber() in onVoiceStart"
llmRef: |
  Array Modulator (Modulator/VoiceStartModulator)

  Returns a per-note modulation value by indexing a 128-entry slider pack with the MIDI note number. Each slider corresponds to one MIDI note (0-127), with values in the 0.0-1.0 range.

  Signal flow:
    noteOn -> extract note number -> sliderPack[noteNumber] -> modulation out

  CPU: negligible, polyphonic (single array lookup per voice start)

  Parameters: none. The slider pack IS the configuration.

  When to use:
    Per-note volume scaling, velocity layers, or any scenario requiring a discrete modulation value for each key. For a continuous curve across the keyboard, use KeyNumber with a table instead.

  Common mistakes:
    All 128 sliders default to 1.0 (transparent) - edit them to create per-note variation.
    The slider pack is fixed at 128 entries and cannot be resized.

  Scripting access:
    Synth.getSliderPackProcessor("id").getSliderPack(0) returns a ScriptSliderPackData handle for runtime manipulation.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: use an array indexed by Message.getNoteNumber() in onVoiceStart.

  See also:
    alternative Velocity - reads MIDI velocity instead of per-note values
    alternative KeyNumber - table-based note mapping with a continuous curve
    disambiguation Constant - fixed value for all notes
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
---
::

![Array Modulator screenshot](/images/v2/reference/audio-modules/arraymodulator.png)

The Array Modulator provides a discrete modulation value for each MIDI note by indexing a 128-entry slider pack with the incoming note number. When a voice starts, it reads the slider value at the position matching the note number (0-127) and outputs that value as the modulation signal. This makes it suitable for per-note volume scaling, split points, or any situation where each key needs an individually tuneable modulation value.

All 128 sliders default to 1.0, so the modulator is transparent until you lower individual values. Unlike the [Key Number Modulator]($MODULES.KeyNumber$) which draws a continuous curve across the keyboard, the Array Modulator gives independent control over every single note.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions:
    sliderPackLookup:
      desc: "Reads the slider value at the given note index from the 128-entry slider pack"
  modulations: {}
---

```
// Array Modulator - per-note modulation from slider pack
// noteOn in -> modulation out (per voice)

onNoteOn() {
    noteNumber = message.noteNumber    // 0-127

    value = sliderPackLookup(noteNumber)    // 0.0 - 1.0

    return value
}
```

::

### Scripting Access

The slider pack can be read and written at runtime from HISEScript. Obtain a reference to the underlying data and manipulate individual slider values:

```javascript
const var sp = Synth.getSliderPackProcessor("ArrayModulator1").getSliderPack(0);
sp.setSliderAtIndex(60, 0.5);  // set middle C to half intensity
```

This allows dynamic per-note mapping that can change based on user input or preset logic.

**See also:** $MODULES.Velocity$ -- reads MIDI velocity instead of per-note slider values, $MODULES.KeyNumber$ -- table-based note mapping with a continuous curve instead of per-note discrete values, $MODULES.Constant$ -- also a VoiceStartModulator but outputs a single fixed value for all notes
