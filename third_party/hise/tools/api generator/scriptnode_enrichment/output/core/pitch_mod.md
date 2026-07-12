---
title: Pitch Mod
description: "Receives the polyphonic pitch modulation signal from the parent sound generator as raw pitch factor values."
factoryPath: core.pitch_mod
factory: core
polyphonic: true
tags: [core, modulation, bridge, pitch]
screenshot: /images/v2/reference/scriptnodes/core/pitch_mod.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.extra_mod", type: companion, reason: "Similar bridge node for extra modulation chains" }
  - { id: "core.global_mod", type: alternative, reason: "Receives from GlobalModulatorContainer with mode formula" }
commonMistakes:
  - title: "Assuming normalised 0-1 output range"
    wrong: "Connecting pitch_mod output to a parameter expecting normalised 0-1 values"
    right: "The output is raw pitch factor values (0.5 = octave down, 1.0 = no change, 2.0 = octave up). Connect to a FreqRatio parameter or scale accordingly."
    explanation: "Unlike other modulation bridge nodes, pitch_mod outputs unnormalised pitch ratios. The display shows a normalised view for visual clarity, but the actual modulation output uses raw pitch factors."
llmRef: |
  core.pitch_mod

  Receives the pitch modulation signal from the parent sound generator's pitch modulation chain. Outputs raw pitch factor values (not normalised 0-1). 0.5 = octave down, 1.0 = no change, 2.0 = octave up.

  Signal flow:
    parent pitch mod chain -> raw pitch factor passthrough -> modulation output (unnormalised)
    (optional) -> audio channel 0 when ProcessSignal is enabled

  CPU: negligible, polyphonic

  Parameters:
    ProcessSignal (Disabled / Enabled, default Disabled): Writes pitch signal to audio channel 0.

  When to use:
    - Feeding pitch modulation (vibrato, pitch envelopes) into oscillator FreqRatio parameters
    - Per-voice pitch tracking for FM synthesis or pitch-dependent effects
    - Sample-accurate pitch modulation within the network

  Common mistakes:
    - Output is unnormalised pitch ratios, not 0-1

  See also:
    [companion] core.extra_mod -- bridge for extra modulation chains
    [alternative] core.global_mod -- receives from GlobalModulatorContainer
---

![Pitch Mod screenshot](/images/custom/scriptnode/pitch_mod.png)

The pitch mod node picks up the pitch modulation signal from the parent sound generator's pitch modulation chain and makes it available inside scriptnode. The output is raw pitch factor values -- not normalised to 0-1. A value of 1.0 means no pitch change, 0.5 is one octave down, and 2.0 is one octave up. This makes the output directly compatible with the FreqRatio parameter of oscillator nodes without any conversion.

The node automatically connects to the parent's pitch modulation chain -- no Index parameter is needed. It works in different module contexts:

- In a ScriptnodeSynthesiser, it receives from the synthesiser's own pitch modulation chain.
- In an FX module (Script FX or Polyphonic Script FX), it receives from the sound generator the effect is placed in.

## Signal Path

::signal-path
---
glossary:
  parameters:
    ProcessSignal:
      desc: "When enabled, writes the pitch modulation signal to audio channel 0"
      range: "Disabled / Enabled"
      default: "Disabled"
  functions:
    pitchPassthrough:
      desc: "Forwards the raw pitch factor values without transformation"
---

```
// core.pitch_mod - pitch modulation chain bridge
// pitch mod chain -> modulation output (unnormalised)

process() {
    pitchFactor = readPitchModChain()
    // pitchFactor: 0.5 = -1 octave, 1.0 = unity, 2.0 = +1 octave

    modOutput = pitchPassthrough(pitchFactor)

    if ProcessSignal == Enabled {
        audio[ch0] = pitchFactor
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Output
    params:
      - { name: ProcessSignal, desc: "When enabled, writes the pitch modulation signal to audio channel 0 for further signal-path processing.", range: "Disabled / Enabled", default: "Disabled" }
---
::

### Output format

The modulation output uses unnormalised mode, meaning downstream parameter connections receive the raw pitch factor values directly. This avoids the overhead of converting to normalised range and back, which matters for sample-accurate pitch modulation in FM synthesis and similar applications. The display plotter in the node UI shows a normalised view of the signal for visual clarity, but this is a display-only transformation -- the actual output remains in the pitch factor domain.

As with all modulation bridge nodes, this node is compilable to C++ and works in all module configurations since HISE 5.0.

**See also:** $SN.core.extra_mod$ -- bridge for extra modulation chains, $SN.core.global_mod$ -- receives from GlobalModulatorContainer with mode formula
