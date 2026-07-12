---
title: Global Static Time Variant Modulator
moduleId: GlobalStaticTimeVariantModulator
type: Modulator
subtype: VoiceStartModulator
tags: [routing]
builderPath: b.Modulators.GlobalStaticTimeVariantModulator
screenshot: /images/v2/reference/audio-modules/globalstatictimevariantmodulator.png
cpuProfile:
  baseline: very low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: GlobalModulatorContainer, type: source, reason: "Hosts the source time-variant modulator that this consumer reads from" }
  - { id: GlobalTimeVariantModulator, type: disambiguation, reason: "Continuously tracks the same source type, rather than snapshotting at note-on" }
  - { id: GlobalVoiceStartModulator, type: disambiguation, reason: "Also a VoiceStartModulator consumer, but reads from VoiceStart sources rather than TimeVariant sources" }
commonMistakes:
  - title: "Snapshot is block-quantised only"
    wrong: "Expecting sample-accurate capture of a fast-moving LFO at the exact note-on moment"
    right: "The captured value is the source's last computed block value, quantised to audio block boundaries"
    explanation: "The snapshot is taken from the source's most recent block output, not interpolated to the exact note-on timestamp. For fast-moving sources, the captured value depends on timing relative to block boundaries."
  - title: "Freezes value at note-on"
    wrong: "Confusing this with Global Time Variant Modulator"
    right: "This module freezes the value at note-on (per-voice). Global Time Variant Modulator tracks continuously (monophonic)."
    explanation: "Despite both connecting to TimeVariant sources, they serve completely different purposes. This one creates per-voice variation based on note timing; the other provides continuous monophonic modulation."
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: low
  description: "A global cable with sample-and-hold at voice start, plus optional table lookup"
llmRef: |
  Global Static Time Variant Modulator (VoiceStartModulator)

  Captures the current value of a TimeVariantModulator from a Global Modulator Container at the moment a note starts. Despite connecting to a continuous source, it behaves as a VoiceStartModulator - each voice gets a fixed value frozen at note-on.

  Signal flow:
    Source last block value (from container) -> optional table lookup -> optional inversion (1 - value) -> modulation output (constant per voice)

  CPU: very low. Single value read per voice start event.

  Parameters:
    UseTable (Off/On, default Off) - enables lookup table for value transformation
    Inverted (Off/On, default Off) - inverts the output (1 - value)

  When to use:
    When you want per-voice variation derived from the current state of a continuous modulator (e.g., an LFO). Each note captures the LFO's value at the moment it is triggered, creating timing-dependent per-voice differences.

  Common mistakes:
    Value is block-quantised, not sample-accurate.
    Confusing with GlobalTimeVariantModulator (continuous tracking vs snapshot).

  See also:
    source GlobalModulatorContainer - hosts the source modulator
    disambiguation GlobalTimeVariantModulator - continuous tracking instead of snapshot
    disambiguation GlobalVoiceStartModulator - reads from VoiceStart sources, not TimeVariant
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Global Static Time Variant Modulator screenshot](/images/v2/reference/audio-modules/globalstatictimevariantmodulator.png)

The Global Static Time Variant Modulator captures the current value of a time-variant modulator (such as an LFO) from a Global Modulator Container at the moment a note starts. Despite connecting to a continuous source, it behaves as a voice-start modulator - each voice receives a fixed value frozen at note-on time.

This creates per-voice variation based on when each note is triggered. For example, connecting to a shared LFO means each note captures a different phase of the LFO cycle, producing timing-dependent differences between voices. An optional lookup table and inversion toggle allow local transformation of the captured value. When disconnected (no source selected), the modulator returns 1.0 (pass-through in gain mode).

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
      desc: "Reads the response curve table at the captured value position (0-1 input, 0-1 output)"
    captureSourceValue:
      desc: "Reads the source TimeVariantModulator's last computed block value (block-quantised, not sample-accurate)"
---

```
// Global Static Time Variant Modulator - per-voice, voice-start
// snapshots a TimeVariant source at note-on from Global Modulator Container

onNoteOn() {
    if not connected:
        return 1.0  // pass-through

    value = captureSourceValue()  // last block output, block-quantised

    if UseTable:
        value = tableLookup(value)

    if Inverted:
        value = 1.0 - value

    return value  // constant for this voice's lifetime
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Value Processing
    params:
      - { name: UseTable, desc: "Enables a lookup table to transform the captured value. The table maps the source value (0-1) to an output value (0-1). Applied before inversion.", range: "Off / On", default: "Off" }
      - { name: Inverted, desc: "Inverts the modulation output by computing 1 minus the value. Applied after the table lookup.", range: "Off / On", default: "Off" }
---
::

**See also:** $MODULES.GlobalModulatorContainer$ -- hosts the source time-variant modulator, $MODULES.GlobalTimeVariantModulator$ -- continuously tracks the same source type in real time (monophonic), $MODULES.GlobalVoiceStartModulator$ -- also a voice-start consumer, but reads from voice-start sources (velocity, key number) rather than time-variant sources (LFO)