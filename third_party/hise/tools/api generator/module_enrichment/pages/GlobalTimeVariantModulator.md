---
title: Global Time Variant Modulator
moduleId: GlobalTimeVariantModulator
type: Modulator
subtype: TimeVariantModulator
tags: [routing]
builderPath: b.Modulators.GlobalTimeVariantModulator
screenshot: /images/v2/reference/audio-modules/globaltimevariantmodulator.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: [table enabled]
seeAlso:
  - { id: GlobalModulatorContainer, type: source, reason: "Hosts the source time-variant modulator that this consumer reads from" }
  - { id: GlobalStaticTimeVariantModulator, type: disambiguation, reason: "Also reads from a TimeVariant source, but snapshots the value at note-on instead of continuously tracking" }
forumReferences:
  - id: 3
    title: "Global modulators unavailable in FX plugin master chain"
    summary: "In FX plugin mode, GlobalTimeVariantModulators do not appear in the connection dropdown for master chain effects; enabling Sound Generator in FX plugin export settings and correct container placement fixes it."
    topic: 1583
  - id: 4
    title: "Free-running LFO requires scriptnode Script FX"
    summary: "A GlobalTimeVariantModulator wrapping an LFO restarts on each note-on; implement a non-retriggering LFO as a scriptnode network inside a Script FX module instead."
    topic: 7693
  - id: 5
    title: "Read modulator value from script via GlobalCable callback"
    summary: "Direct polling of a GlobalTimeVariantModulator's value from HISEScript is not supported; attach a GlobalCable and register a callback to receive updates."
    topic: 9641
commonMistakes:
  - title: "Inverted doesn't work with UseTable"
    wrong: "Using the Inverted toggle with UseTable enabled and expecting inversion to work"
    right: "When UseTable is enabled, inversion has no effect. Use a table with an inverted curve instead."
    explanation: "Due to an internal issue, inversion is not applied when the table is active. Draw an inverted curve in the table as a workaround."
  - title: "Disconnected modulator outputs constant 1.0"
    wrong: "Adding a Global Time Variant consumer without selecting a source"
    right: "Select a source from the Connection dropdown"
    explanation: "When disconnected, the modulator outputs a constant 1.0 and does not track any source."
  - title: "Container must be above consumers in the module tree"
    wrong: "Placing the Global Modulator Container below the sound generators that use this modulator"
    right: "Place the Global Modulator Container above all consumers in the module tree"
    explanation: "HISE processes modules top-down. If the container is below its consumers, the modulation source has not yet rendered when the consumer tries to read it, resulting in silent modulation with no error. [1]($FORUM_REF.1015$)"
  - title: "setNoteNumber() breaks global modulator lookup"
    wrong: "Using `Message.setNoteNumber()` in a MIDI script when global modulators are active"
    right: "Use `Message.setTransposeAmount()` to redirect notes while preserving the event association"
    explanation: "Global modulators identify events by note number and event ID. Calling `setNoteNumber()` changes the note number after the container has processed the event, breaking the association. `setTransposeAmount()` preserves the original note number. [2]($FORUM_REF.6390$)"
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: low
  description: "A global cable receiving time-variant buffer with optional table lookup node"
llmRef: |
  Global Time Variant Modulator (TimeVariantModulator)

  Continuously reads a monophonic time-variant modulation buffer from a source TimeVariantModulator in a Global Modulator Container. Tracks the source in real time, block by block.

  Signal flow:
    Source buffer (from container, per-block) -> optional table lookup (per-sample) -> optional inversion -> modulation output

  CPU: low. Buffer copy per block. Optional per-sample table lookup adds moderate cost.

  Parameters:
    UseTable (Off/On, default Off) - enables lookup table for value transformation
    Inverted (Off/On, default Off) - inverts the output (1 - value). Note: does not work when UseTable is enabled.

  When to use:
    When the same continuous modulation source (LFO, etc.) needs to drive parameters across multiple targets in real time.

  Common mistakes:
    Inversion does not work when UseTable is enabled.
    No source selected - outputs constant 1.0.
    Container must be above all consumers in the module tree (top-down processing order).
    setNoteNumber() breaks event association - use setTransposeAmount() instead.
    LFO source restarts on note-on; use scriptnode Script FX for free-running LFO.
    Read values from script via GlobalCable callback, not direct polling.

  See also:
    source GlobalModulatorContainer - hosts the source modulator
    disambiguation GlobalStaticTimeVariantModulator - snapshots at note-on instead of continuous tracking
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Global Time Variant Modulator screenshot](/images/v2/reference/audio-modules/globaltimevariantmodulator.png)

The Global Time Variant Modulator continuously reads a monophonic modulation buffer from a source time-variant modulator (such as an LFO) hosted in a Global Modulator Container. It tracks the source in real time, copying the full per-block buffer each audio cycle. This makes it suitable for any continuous modulation that needs to drive multiple targets simultaneously.

An optional lookup table can reshape the modulation values per-sample. The inversion toggle works in the non-table path but does not function when the table is active - use an inverted table curve as a workaround.

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
      desc: "Inverts the modulation output (1 - value). Does not work when UseTable is enabled."
      range: "Off / On"
      default: "Off"
  functions:
    tableLookup:
      desc: "Reads the response curve table at the source value position (0-1 input, 0-1 output), applied per-sample"
    copySourceBuffer:
      desc: "Copies the pre-computed time-variant buffer from the source in the Global Modulator Container"
---

```
// Global Time Variant Modulator - monophonic, time-variant
// reads from source TimeVariantModulator in Global Modulator Container

perBlock() {
    if not connected:
        fill(output, 1.0)  // pass-through
        return

    if UseTable:
        for each sample:
            output[sample] = tableLookup(sourceBuffer[sample])
        // Note: Inverted has no effect in this path
    else:
        copySourceBuffer()
        if Inverted:
            for each sample:
                output[sample] = 1.0 - output[sample]
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Value Processing
    params:
      - { name: UseTable, desc: "Enables a lookup table to transform the incoming modulation values per-sample. The table maps each source value (0-1) to an output value (0-1).", range: "Off / On", default: "Off" }
      - { name: Inverted, desc: "Inverts the modulation output by computing 1 minus the value. Only works when UseTable is disabled. When UseTable is enabled, inversion has no effect.", range: "Off / On", default: "Off" }
---
::

### FX Plugin Mode

In FX plugin exports, global modulators are not available in the master chain connection dropdown by default. Enable the Sound Generator option in the FX plugin export settings and place the Global Modulator Container correctly to make the connection available. [3]($FORUM_REF.1583$)

### Free-Running LFO

A Global Time Variant Modulator wrapping an LFO restarts on each note-on. For a free-running LFO that does not retrigger, implement the oscillator as a scriptnode network inside a Script FX module instead. [4]($FORUM_REF.7693$)

### Scripting Access

Reading the current value of a Global Time Variant Modulator from HISEScript is not supported directly. Attach a GlobalCable to the modulator and register a callback to receive value updates on the script side. [5]($FORUM_REF.9641$)

**See also:** $MODULES.GlobalModulatorContainer$ -- hosts the source time-variant modulator, $MODULES.GlobalStaticTimeVariantModulator$ -- also reads from a time-variant source, but snapshots the value at note-on for per-voice variation