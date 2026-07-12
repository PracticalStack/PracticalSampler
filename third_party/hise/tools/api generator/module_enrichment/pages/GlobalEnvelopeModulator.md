---
title: Global Envelope Modulator
moduleId: GlobalEnvelopeModulator
type: Modulator
subtype: EnvelopeModulator
tags: [routing]
builderPath: b.Modulators.GlobalEnvelopeModulator
screenshot: /images/v2/reference/audio-modules/globalenvelopemodulator.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: [table enabled]
seeAlso:
  - { id: GlobalModulatorContainer, type: source, reason: "Hosts the source envelope that this modulator reads from" }
  - { id: MatrixModulator, type: alternative, reason: "Combines multiple global modulators into one output with per-connection modes, rather than reading a single source" }
commonMistakes:
  - title: "Disconnected modulator outputs constant value"
    wrong: "Adding a Global Envelope without selecting a source in the dropdown"
    right: "Select a source envelope from the Connection dropdown"
    explanation: "When disconnected, the modulator outputs a constant 1.0 (gain mode) or 0.0 (pitch mode) and does not track any envelope."
  - title: "Inverted toggle has no effect"
    wrong: "Expecting the Inverted toggle to invert the envelope signal"
    right: "The Inverted parameter has no effect"
    explanation: "The Inverted toggle is visible in the editor but has no effect on the output. Use a table with an inverted curve as a workaround."
  - title: "Requires Uniform Voice Handler"
    wrong: "Using a Global Envelope Modulator without enabling the Uniform Voice Handler"
    right: "Call `Synth.setUseUniformVoiceHandler()` at the root level"
    explanation: "Global envelopes match voices via event IDs. Without the Uniform Voice Handler, event IDs are inconsistent across sound generators and the envelope assignment silently fails. [1]($FORUM_REF.9499$)"
  - title: "Do not use inside a Synth Group"
    wrong: "Placing a Global Envelope consumer inside a Synth Group"
    right: "Use a Synth Chain as the host container instead"
    explanation: "Synth Group starts multiple voices per event, violating the one-voice-per-event constraint that the Uniform Voice Handler depends on. [2]($FORUM_REF.9499$)"
forumReferences:
  - id: 1
    title: "Requires Uniform Voice Handler for correct voice sync"
    summary: "Global envelopes rely on consistent event IDs; without a Uniform Voice Handler the envelope assignment silently fails. Synth Group also breaks this by starting multiple voices per event."
    topic: 9499
  - id: 3
    title: "Arpeggiator must be in the Master Chain to trigger global envelopes"
    summary: "An arpeggiator inside a child synth's MIDI chain produces notes never seen by the GlobalModulatorContainer; only events passing through the master chain can trigger a new envelope voice."
    topic: 13000
  - id: 4
    title: "Setting intensity to zero does not kill notes"
    summary: "Voice lifetime is determined by the source envelope in the container; setting consumer intensity to zero silences output but the voice keeps running until the source envelope finishes."
    topic: 14176
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: low
  description: "A global cable receiving envelope values with optional table lookup node"
llmRef: |
  Global Envelope Modulator (EnvelopeModulator)

  Reads per-voice envelope values from a source envelope hosted in a Global Modulator Container. Acts as a lightweight proxy - the actual envelope shape is determined entirely by the source.

  Signal flow:
    Source envelope buffer (from container) -> optional table lookup (per-sample) -> modulation output
    Voice sync uses event IDs, not voice indices, so it works across different Sound Generators.

  CPU: low. Copies pre-computed buffer from source. Optional per-sample table lookup adds moderate cost.

  Parameters:
    Monophonic (Off/On, default dynamic) - affects voice management, not envelope shape
    Retrigger (Off/On, default On) - affects voice management, not envelope shape
    UseTable (Off/On, default Off) - enables lookup table for value transformation
    Inverted (Off/On, default Off) - vestigial, has no effect

  When to use:
    When the same envelope shape needs to modulate parameters across multiple Sound Generators. Place the source envelope in a Global Modulator Container, then add this consumer wherever needed.

  Common mistakes:
    No source selected - outputs constant 1.0.
    Inverted toggle has no effect.
    Requires Uniform Voice Handler (Synth.setUseUniformVoiceHandler()) or voice sync fails silently.
    Do not use inside a Synth Group (starts multiple voices per event, breaking voice sync).
    Arpeggiator must be in the Master Chain to trigger global envelope voices.
    Setting intensity to zero silences output but does not kill the voice.

  See also:
    source GlobalModulatorContainer - hosts the source envelope
    alternative MatrixModulator - combines multiple global sources
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Global Envelope Modulator screenshot](/images/v2/reference/audio-modules/globalenvelopemodulator.png)

The Global Envelope Modulator reads per-voice envelope values from a source envelope hosted in a Global Modulator Container. It acts as a lightweight proxy - the actual envelope shape (attack, decay, sustain, release) is determined entirely by the source. Each voice in this modulator is synchronised to the corresponding voice in the source via event matching, so it works correctly even when the consumer and source belong to different Sound Generators with independent voice allocation.

An optional lookup table can reshape the envelope values per-sample before they reach the modulation output. This is useful for applying custom response curves to a shared envelope without modifying the original.

## Signal Path

::signal-path
---
glossary:
  parameters:
    UseTable:
      desc: "Enables the lookup table for value transformation"
      range: "Off / On"
      default: "Off"
  functions:
    tableLookup:
      desc: "Reads the response curve table at the envelope value position (0-1 input, 0-1 output)"
    copySourceBuffer:
      desc: "Copies the pre-computed envelope buffer from the source in the Global Modulator Container"
---

```
// Global Envelope Modulator - per-voice, envelope
// reads from source envelope in Global Modulator Container

onNoteOn() {
    // Match this voice to the source envelope via event ID
}

perVoiceBlock() {
    if not connected:
        output = 1.0  // pass-through
        return

    copySourceBuffer()

    if UseTable:
        for each sample:
            output[sample] = tableLookup(sourceValue[sample])
    else:
        output = sourceBuffer
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Value Processing
    params:
      - { name: UseTable, desc: "Enables a lookup table to transform the incoming envelope values. The table maps each sample value (0-1) to an output value (0-1) before the modulation output.", range: "Off / On", default: "Off" }
      - { name: Inverted, desc: "This parameter has no effect.", range: "Off / On", default: "Off" }
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "Enables monophonic mode. Affects how voices are managed locally but does not change the envelope shape, which is always determined by the source.", range: "Off / On", default: "(dynamic)" }
      - { name: Retrigger, desc: "Restarts the envelope proxy when a new note is triggered in monophonic mode. Does not affect the source envelope's behaviour.", range: "Off / On", default: "On" }
---
::

### Arpeggiator Placement

When using an arpeggiator with global envelopes, the arpeggiator must be placed in the Master Chain's MIDI processor list. An arpeggiator placed inside a child synth (e.g., in a Sine Generator's MIDI chain) produces notes that are never seen by the Global Modulator Container, so those notes cannot trigger a new envelope voice. [3]($FORUM_REF.13000$)

### Voice Lifetime and Intensity

Setting the modulation intensity to zero silences the output but does not kill the voice. Voice lifetime is determined by the source envelope in the container - the voice keeps running until the source envelope finishes its release phase. [4]($FORUM_REF.14176$)

**See also:** $MODULES.GlobalModulatorContainer$ -- hosts the source envelope that this modulator reads from, $MODULES.MatrixModulator$ -- combines multiple global modulators into one output with per-connection modes