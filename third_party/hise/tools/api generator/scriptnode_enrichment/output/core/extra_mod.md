---
title: Extra Mod
description: "Picks up a modulation signal from an extra modulation chain of the parent module."
factoryPath: core.extra_mod
factory: core
polyphonic: true
tags: [core, modulation, bridge]
screenshot: /images/v2/reference/scriptnodes/core/extra_mod.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.global_mod", type: alternative, reason: "Receives from GlobalModulatorContainer instead of extra mod chains" }
  - { id: "core.pitch_mod", type: companion, reason: "Similar bridge node for the pitch modulation chain" }
  - { id: "core.matrix_mod", type: alternative, reason: "Dual-source modulation with matrix processing features" }
commonMistakes:
  - title: "Missing preprocessor for mod slots"
    wrong: "Adding core.extra_mod without defining the preprocessor for the module type"
    right: "Set the appropriate preprocessor (e.g. HISE_NUM_SCRIPTNODE_FX_MODS=1) in the project settings."
    explanation: "Extra modulation chains must be enabled per module type via preprocessor definitions. Without them, no modulation slots are available and the node has nothing to connect to."
  - title: "Connecting root parameter to same index"
    wrong: "Connecting a root parameter target and an extra_mod node to the same modulation index"
    right: "Use either a root parameter connection or an extra_mod node for a given index, not both."
    explanation: "These two modes are mutually exclusive. If both are active on the same index, a warning icon appears and the behaviour is undefined."
llmRef: |
  core.extra_mod

  Picks up a modulation signal from an extra modulation chain of the parent module and exposes it inside scriptnode. The signal passes through unmodified (raw mode) -- no intensity scaling or mode formula is applied. The Index parameter selects which extra modulation chain slot to read from.

  Signal flow:
    parent module's extra mod chain -> raw passthrough -> modulation output (normalised 0-1)
    (optional) -> audio channel 0 when ProcessSignal is enabled

  CPU: negligible, polyphonic

  Parameters:
    Index (0 - 16, default 0): Selects which extra modulation chain to read from.
    ProcessSignal (Disabled / Enabled, default Disabled): Writes raw modulation to audio channel 0.

  When to use:
    - Bringing external modulation (LFOs, envelopes) into a scriptnode network
    - Per-voice modulation from the parent module's extra mod chains
    - Sample-accurate modulation of parameters within the network

  Common mistakes:
    - Must define preprocessor for extra mod slot count per module type
    - Cannot use root parameter connection and extra_mod on the same index

  See also:
    [alternative] core.global_mod -- receives from GlobalModulatorContainer
    [companion] core.pitch_mod -- bridge for pitch modulation chain
    [alternative] core.matrix_mod -- dual-source modulation with matrix features
---

![Extra Mod screenshot](/images/custom/scriptnode/extra_mod.png)

The extra mod node bridges HISE's modulation system into scriptnode by picking up signals from the extra modulation chains of the parent module. These are the additional modulation slots that a scriptnode module can expose -- for example, a filter's frequency modulation or a custom per-parameter modulation input. The signal passes through unmodified, preserving the raw normalised value (0-1) from the modulation chain.

To use this node, the parent module type must have extra modulation slots enabled via preprocessor definitions (e.g. `HISE_NUM_SCRIPTNODE_FX_MODS` for Script FX modules). Scriptnode synthesisers have two slots enabled by default. Once configured, the modulation slots appear on the parent module and any HISE modulators added to those slots are picked up by this node.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Index:
      desc: "Selects which extra modulation chain slot to read from"
      range: "0 - 16"
      default: "0"
    ProcessSignal:
      desc: "When enabled, writes the modulation signal to audio channel 0"
      range: "Disabled / Enabled"
      default: "Disabled"
  functions:
    rawPassthrough:
      desc: "Forwards the modulation signal without any transformation"
---

```
// core.extra_mod - extra modulation chain bridge
// mod chain signal -> modulation output

process() {
    signal = readModChain(Index)

    // Raw passthrough -- no scaling or mode formula
    modOutput = rawPassthrough(signal)

    if ProcessSignal == Enabled {
        audio[ch0] = signal
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: Index, desc: "Selects which extra modulation chain to read from within the parent module. Index 0 reads the first chain, index 1 the second, and so on.", range: "0 - 16", default: "0" }
      - { name: ProcessSignal, desc: "When enabled, writes the raw modulation signal to audio channel 0 for further processing in the signal path.", range: "Disabled / Enabled", default: "Disabled" }
---
::

### Root parameter interaction

The extra mod node supports an interaction with root parameters: if a root parameter is configured with the Combined modulation mode and associated with the same index as an extra_mod node, the root parameter controls the base value of the modulation chain rather than directly setting the target. This enables modulation display on UI knobs while the actual modulation happens at full resolution inside the network.

### Compilation

If you plan to compile the DSP network to a C++ node, the corresponding hardcoded module preprocessor variables must also be set (e.g. `NUM_HARDCODED_FX_MODS`).

**See also:** $SN.core.global_mod$ -- receives from GlobalModulatorContainer instead of extra mod chains, $SN.core.pitch_mod$ -- bridge for the pitch modulation chain, $SN.core.matrix_mod$ -- dual-source modulation with matrix processing features
