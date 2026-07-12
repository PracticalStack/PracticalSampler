---
title: Global Voice Start Modulator
moduleId: GlobalVoiceStartModulator
type: Modulator
subtype: VoiceStartModulator
tags: [routing]
builderPath: b.Modulators.GlobalVoiceStartModulator
screenshot: /images/v2/reference/audio-modules/globalvoicestartmodulator.png
cpuProfile:
  baseline: very low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: GlobalModulatorContainer, type: source, reason: "Hosts the source voice-start modulator that this consumer reads from" }
  - { id: GlobalStaticTimeVariantModulator, type: disambiguation, reason: "Also a VoiceStartModulator consumer, but reads from a TimeVariant source rather than a VoiceStart source" }
forumReferences:
  - id: 4
    title: "Use setTransposeAmount() to preserve event association"
    summary: "Redirecting notes with Message.setTransposeAmount() keeps the original event ID intact so global modulators resolve correctly; Synth.playNote() breaks the association."
    topic: 2259
  - id: 5
    title: "Replace with ScriptVoiceStartModulator for programmatic notes"
    summary: "When notes are generated programmatically, replace the global modulator with a ScriptVoiceStartModulator that reads Message.getVelocity() directly."
    topic: 10815
commonMistakes:
  - title: "Same note gets same value"
    wrong: "Expecting different values for multiple voices playing the same note"
    right: "All voices on the same MIDI note number receive the same value"
    explanation: "Voice-start values are indexed by MIDI note number (0-127), not by voice. Retriggering the same note returns the same value from the source."
  - title: "Does not work with artificial notes from Synth.playNote()"
    wrong: "Using a Global Voice Start Modulator in a signal chain that generates notes via `Synth.playNote()`"
    right: "Use `Message.setTransposeAmount()` to redirect notes, or replace with a $MODULES.ScriptVoiceStartModulator$"
    explanation: "Notes created by `Synth.playNote()` are generated after the container has already processed the original event. The artificial note has a new event ID with no stored voice-start value, producing silence or undefined output. [1]($FORUM_REF.2259$) [2]($FORUM_REF.10815$)"
  - title: "Arpeggiator causes note dropouts"
    wrong: "Using a Global Voice Start Modulator alongside an arpeggiator"
    right: "Use a local voice-start modulator directly on each sound generator, or disable global velocity modulation when the arpeggiator is active"
    explanation: "Arpeggiator-generated notes are artificial events that lack stored values in the global container, causing some notes to play silently. [3]($FORUM_REF.6271$)"
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: low
  description: "A global cable receiving voice-start values with optional table lookup node"
llmRef: |
  Global Voice Start Modulator (VoiceStartModulator)

  Reads a per-note voice-start value from a source VoiceStartModulator in a Global Modulator Container. Values are indexed by MIDI note number.

  Signal flow:
    Source value (per-note, from container) -> optional table lookup -> optional inversion (1 - value) -> modulation output

  CPU: very low. Single array lookup per voice start event.

  Parameters:
    UseTable (Off/On, default Off) - enables lookup table for value transformation
    Inverted (Off/On, default Off) - inverts the output (1 - value)

  When to use:
    When the same voice-start modulation (velocity, random, key number, etc.) needs to drive parameters across multiple Sound Generators.

  Common mistakes:
    All voices on the same note number get the same value.
    No source selected - outputs constant 1.0.
    Does not work with artificial notes from Synth.playNote() - use Message.setTransposeAmount() or a ScriptVoiceStartModulator instead.
    Arpeggiator-generated notes lack stored values, causing dropouts.

  See also:
    source GlobalModulatorContainer - hosts the source modulator
    disambiguation GlobalStaticTimeVariantModulator - also VoiceStartModulator but reads from TimeVariant source
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Global Voice Start Modulator screenshot](/images/v2/reference/audio-modules/globalvoicestartmodulator.png)

The Global Voice Start Modulator reads a per-note value from a source VoiceStartModulator hosted in a Global Modulator Container. When a note is triggered, the consumer looks up the value the source computed for that MIDI note number and uses it as its own voice-start modulation value.

An optional lookup table and inversion toggle allow local transformation of the shared value without affecting the original source or other consumers reading from the same modulator.

## Signal Path

::signal-path
---
glossary:
  parameters:
    UseTable:
      desc: "Enables the lookup table for value transformation"
      range: "Off / On"
      default: "Off"
    Inverted:
      desc: "Inverts the modulation output (1 - value)"
      range: "Off / On"
      default: "Off"
  functions:
    tableLookup:
      desc: "Reads the response curve table at the source value position (0-1 input, 0-1 output)"
    lookupSourceValue:
      desc: "Retrieves the pre-computed voice-start value from the container, indexed by MIDI note number"
---

```
// Global Voice Start Modulator - per-voice, voice-start
// reads from source VoiceStartModulator in Global Modulator Container

onNoteOn() {
    if not connected:
        return 1.0  // pass-through

    value = lookupSourceValue(noteNumber)

    if UseTable:
        value = tableLookup(value)

    if Inverted:
        value = 1.0 - value

    return value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Value Processing
    params:
      - { name: UseTable, desc: "Enables a lookup table to transform the incoming voice-start value. The table maps the source value (0-1) to an output value (0-1). Applied before inversion.", range: "Off / On", default: "Off" }
      - { name: Inverted, desc: "Inverts the modulation output by computing 1 minus the value. Applied after the table lookup.", range: "Off / On", default: "Off" }
---
::

### Working with Note Remapping

When a script needs to redirect incoming notes to different pitches, use `Message.setTransposeAmount()` rather than blocking the original event and calling `Synth.playNote()`. This preserves the original event ID so the global modulator container can correctly associate the voice-start value with the note. [4]($FORUM_REF.2259$)

For instruments that generate notes programmatically (arpeggios, strum scripts, chord generators), consider replacing this module with a $MODULES.ScriptVoiceStartModulator$ that reads `Message.getVelocity()` directly. This avoids the event ID lookup entirely. [5]($FORUM_REF.10815$)

**See also:** $MODULES.GlobalModulatorContainer$ -- hosts the source voice-start modulator, $MODULES.GlobalStaticTimeVariantModulator$ -- also a voice-start consumer, but reads from a time-variant source (snapshots an LFO or similar at note-on)