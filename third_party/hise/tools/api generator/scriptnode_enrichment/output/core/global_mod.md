---
title: Global Mod
description: "Receives a modulation signal from the GlobalModulatorContainer and exposes it inside scriptnode."
factoryPath: core.global_mod
factory: core
polyphonic: true
tags: [core, modulation, bridge, global-modulator]
screenshot: /images/v2/reference/scriptnodes/core/global_mod.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.extra_mod", type: alternative, reason: "Receives from extra mod chains instead of GlobalModulatorContainer" }
  - { id: "core.matrix_mod", type: alternative, reason: "Dual-source variant with aux modulation and matrix features" }
  - { id: "GlobalModulatorContainer", type: module, reason: "Module-tree container that hosts the modulators this node reads from" }
commonMistakes:
  - title: "Wrong Index for target modulator"
    wrong: "Guessing the Index value without checking the GlobalModulatorContainer order"
    right: "The Index corresponds to the position of the modulator in the GlobalModulatorContainer's child list (0-based)."
    explanation: "Index 0 maps to the first child modulator in the GlobalModulatorContainer, index 1 to the second, and so on. Reordering modulators in the container changes which index maps to which modulator."
llmRef: |
  core.global_mod

  Receives a modulation signal from a modulator in the GlobalModulatorContainer and exposes it inside scriptnode. Applies a configurable mode formula (Gain, Unipolar, or Bipolar) with Value and Intensity parameters.

  Signal flow:
    GlobalModulatorContainer child -> mode formula (Gain/Unipolar/Bipolar) -> modulation output (0-1)
    (optional) -> audio channel 0 when ProcessSignal is enabled

  CPU: low, polyphonic

  Parameters:
    Index (0 - 16, default 0): Selects the modulator slot in the GlobalModulatorContainer.
    Value (0.0 - 1.0, default 1.0): Base value for the modulation formula.
    ProcessSignal (Disabled / Enabled, default Disabled): Writes modulation to audio channel 0.
    Mode (Gain / Unipolar / Bipolar, default Gain): Selects the modulation formula.
    Intensity (-1.0 - 1.0, default 1.0): Modulation depth; negative values invert.

  When to use:
    - Hardwiring a single global modulator connection into a scriptnode network
    - Per-voice modulation from time-variant or envelope modulators in the GlobalModulatorContainer
    - When the modulation needs scaling via Mode/Intensity before reaching its target

  Common mistakes:
    - Index must match the child position in the GlobalModulatorContainer

  See also:
    [alternative] core.extra_mod -- receives from extra mod chains
    [alternative] core.matrix_mod -- dual-source with aux and matrix features
    [module] GlobalModulatorContainer -- module-tree container that hosts the modulators this node reads from
---

![Global Mod screenshot](/images/custom/scriptnode/global_mod.png)

The global mod node bridges HISE's [GlobalModulatorContainer]($MODULES.GlobalModulatorContainer$) into scriptnode by picking up the signal from one of its child modulators. Unlike [core.extra_mod]($SN.core.extra_mod$) which passes the signal through unmodified, this node applies a configurable mode formula with Value and Intensity controls, giving direct control over how the modulation signal is shaped before it reaches its target.

Depending on your project architecture, it may make more sense to use [core.extra_mod]($SN.core.extra_mod$) with global modulators added to the extra modulation chain. However, if you want to hardwire a single global modulation connection directly into the DSP network, this node is the right choice.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Index:
      desc: "Selects which modulator in the GlobalModulatorContainer to read from"
      range: "0 - 16"
      default: "0"
    Value:
      desc: "Base value for the modulation formula (centre point or ceiling)"
      range: "0.0 - 1.0"
      default: "1.0"
    Mode:
      desc: "Selects the modulation formula: Gain, Unipolar, or Bipolar"
      range: "Gain / Unipolar / Bipolar"
      default: "Gain"
    Intensity:
      desc: "Modulation depth; negative values invert the modulation direction"
      range: "-1.0 - 1.0"
      default: "1.0"
    ProcessSignal:
      desc: "When enabled, writes the modulation signal to audio channel 0"
      range: "Disabled / Enabled"
      default: "Disabled"
  functions:
    applyMode:
      desc: "Applies the selected mode formula to combine the raw signal with Value and Intensity"
---

```
// core.global_mod - global modulator bridge
// GlobalModulatorContainer signal -> modulation output

process() {
    signal = readGlobalMod(Index)

    if Mode == Gain {
        output = Value - Intensity + Intensity * signal * Value
    } else if Mode == Unipolar {
        output = Value + signal * Intensity
    } else if Mode == Bipolar {
        output = Value + (2 * signal - 1) * Intensity
    }

    modOutput = applyMode(output)

    if ProcessSignal == Enabled {
        audio[ch0] = output
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Source
    params:
      - { name: Index, desc: "Selects which modulator in the GlobalModulatorContainer to read from. Corresponds to the child position (0-based).", range: "0 - 16", default: "0" }
  - label: Modulation
    params:
      - { name: Value, desc: "Base value for the modulation formula. In Gain mode, acts as the ceiling. In Unipolar/Bipolar modes, acts as the centre point.", range: "0.0 - 1.0", default: "1.0" }
      - { name: Mode, desc: "Selects how the raw signal is combined with Value and Intensity.", range: "Gain / Unipolar / Bipolar", default: "Gain" }
      - { name: Intensity, desc: "Modulation depth. Negative values invert the modulation direction.", range: "-1.0 - 1.0", default: "1.0" }
  - label: Output
    params:
      - { name: ProcessSignal, desc: "When enabled, writes the processed modulation signal to audio channel 0 for further signal-path processing.", range: "Disabled / Enabled", default: "Disabled" }
---
::

### Output behaviour

In Gain mode with Value = 1.0 and Intensity = 1.0, the output equals the raw modulation signal. This is the most common configuration for straightforward modulation passthrough with scaling.

The output is clamped to 0-1 in all modes. Parameter changes are smoothed to prevent clicks. When the GlobalModulatorContainer is not connected or the selected slot is empty, the output defaults to the base Value with no modulation applied.

This node is compilable to C++ nodes (since HISE 5.0), so it properly connects to global modulators in compiled networks.

**See also:** $SN.core.extra_mod$ -- receives from extra mod chains instead of GlobalModulatorContainer, $SN.core.matrix_mod$ -- dual-source variant with aux modulation and matrix features, $MODULES.GlobalModulatorContainer$ -- module-tree container that hosts the modulators this node reads from
