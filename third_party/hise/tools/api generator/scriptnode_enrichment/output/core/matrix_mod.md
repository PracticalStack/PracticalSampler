---
title: Matrix Mod
description: "A dual-source modulation bridge with modulation matrix features for combining two global modulators."
factoryPath: core.matrix_mod
factory: core
polyphonic: true
tags: [core, modulation, bridge, global-modulator, matrix]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.global_mod", type: disambiguation, reason: "Single-source variant without aux modulation" }
  - { id: "core.extra_mod", type: alternative, reason: "Simpler bridge for extra modulation chains" }
commonMistakes:
  - title: "Forgetting to set Intensity above zero"
    wrong: "Adding the node and expecting modulation output with default settings"
    right: "Set Intensity to a non-zero value. The default of 0.0 means no modulation effect."
    explanation: "The Intensity parameter defaults to 0.0, which means the modulation output has no effect regardless of the source signal. Increase Intensity to hear the modulation."
  - title: "Expecting output clamped to 0-1"
    wrong: "Assuming downstream parameters receive values in the 0-1 range"
    right: "Unlike core.global_mod, this node does not clamp output. Downstream nodes must handle values outside 0-1."
    explanation: "The mode formulas in matrix_mod do not clamp the result to 0-1 as global_mod does. In Unipolar and Bipolar modes especially, the output may exceed this range."
llmRef: |
  core.matrix_mod

  A dual-source modulation node that reads two modulators from the GlobalModulatorContainer. The source provides the main signal; the aux modulator dynamically scales the modulation intensity. Combines them using Scale, Unipolar, or Bipolar mode with inversion and zero-position options. Output is NOT clamped to 0-1.

  Signal flow:
    source modulator + aux modulator -> mode formula (Scale/Unipolar/Bipolar) -> audio ch0 + modulation output

  CPU: low, polyphonic

  Parameters:
    SourceIndex (-1 - 64, default -1): Primary source (-1 = disconnected).
    Intensity (-1.0 - 1.0, default 0.0): Overall modulation depth (base value for aux).
    Mode (Scale / Unipolar / Bipolar, default Scale): Combination formula.
    Inverted (Normal / Inverted, default Normal): Flips source signal.
    AuxIndex (-1 - 64, default -1): Auxiliary source (-1 = disconnected).
    AuxIntensity (0.0 - 1.0, default 0.0): How much aux signal affects intensity.
    ZeroPosition (Left / Center, default Left): Modulation centre for Scale mode.

  When to use:
    - Modulation depth that varies dynamically (e.g. velocity controlling vibrato depth)
    - Complex modulation routing with source inversion and centre offset
    - When global_mod's single-source approach is insufficient

  Common mistakes:
    - Default Intensity is 0.0 -- must increase for modulation to take effect
    - Output is not clamped to 0-1

  See also:
    [disambiguation] core.global_mod -- single-source variant
    [alternative] core.extra_mod -- simpler bridge for extra mod chains
---

The matrix mod node extends [core.global_mod]($SN.core.global_mod$) with dual-source modulation from the [GlobalModulatorContainer]($MODULES.GlobalModulatorContainer$). It reads two modulators: a primary source and an auxiliary source. The aux signal dynamically modulates the intensity of the source signal, enabling scenarios such as velocity-controlled vibrato depth or expression-dependent modulation amount.

The node offers three combination modes (Scale, Unipolar, Bipolar) along with source inversion and a zero-position offset for Scale mode. Unlike global_mod, the output is not clamped to 0-1, so downstream nodes should account for values outside that range. Both sources start disconnected (index -1) and the Intensity defaults to 0.0, so the node must be explicitly configured before it produces any modulation effect.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SourceIndex:
      desc: "Selects the primary modulation source from the GlobalModulatorContainer (-1 = disconnected)"
      range: "-1 - 64"
      default: "-1"
    AuxIndex:
      desc: "Selects the auxiliary modulation source (-1 = disconnected)"
      range: "-1 - 64"
      default: "-1"
    Intensity:
      desc: "Overall modulation depth; sets the base value for the aux channel"
      range: "-1.0 - 1.0"
      default: "0.0"
    AuxIntensity:
      desc: "Controls how much the aux signal affects the intensity scaling"
      range: "0.0 - 1.0"
      default: "0.0"
    Mode:
      desc: "Selects the combination formula: Scale, Unipolar, or Bipolar"
      range: "Scale / Unipolar / Bipolar"
      default: "Scale"
    Inverted:
      desc: "Flips the source signal (1 - signal) before the mode formula"
      range: "Normal / Inverted"
      default: "Normal"
    ZeroPosition:
      desc: "Sets the modulation centre point in Scale mode (Left = 0.0, Center = 0.5)"
      range: "Left / Center"
      default: "Left"
  functions:
    combineSourceAux:
      desc: "Combines source and aux signals using the selected mode with inversion and zero-position"
---

```
// core.matrix_mod - dual-source modulation bridge
// source + aux -> mode formula -> audio ch0 + mod output

process() {
    source = readGlobalMod(SourceIndex)
    aux = readGlobalMod(AuxIndex)

    if Inverted == Inverted {
        source = 1.0 - source
    }

    // Aux scales the effective intensity
    effectiveIntensity = (1 - AuxIntensity) + AuxIntensity * aux

    output = combineSourceAux(source, effectiveIntensity, Mode, ZeroPosition)

    audio[ch0] = output
    modOutput = output
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Source
    params:
      - { name: SourceIndex, desc: "Selects the primary modulation source from the GlobalModulatorContainer. -1 means disconnected.", range: "-1 - 64", default: "-1" }
      - { name: Inverted, desc: "Flips the source signal before the mode formula is applied.", range: "Normal / Inverted", default: "Normal" }
  - label: Modulation
    params:
      - { name: Intensity, desc: "Overall modulation depth. Acts as the base value for the aux channel. Must be non-zero for modulation to take effect.", range: "-1.0 - 1.0", default: "0.0" }
      - { name: Mode, desc: "Selects how source and aux signals are combined with the input.", range: "Scale / Unipolar / Bipolar", default: "Scale" }
      - { name: ZeroPosition, desc: "Sets the modulation centre point in Scale mode. Left uses 0.0, Center uses 0.5.", range: "Left / Center", default: "Left" }
  - label: Auxiliary
    params:
      - { name: AuxIndex, desc: "Selects the auxiliary modulation source from the GlobalModulatorContainer. -1 means disconnected.", range: "-1 - 64", default: "-1" }
      - { name: AuxIntensity, desc: "Controls how much the aux modulator signal affects the intensity scaling. At 0 the aux has no effect; at 1 the aux fully modulates the intensity.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Mode details

The three modes work differently from [core.global_mod]($SN.core.global_mod$):

- **Scale:** The aux-modulated intensity scales the source signal multiplicatively, with an optional centre offset from ZeroPosition. When ZeroPosition is Center, modulation is centred around 0.5.
- **Unipolar:** The source signal is multiplied by the aux-modulated intensity and added to the input.
- **Bipolar:** The source is converted to a bipolar range (-1 to +1), scaled by the aux-modulated intensity, and added to the input.

### Voice tracking

The node can track envelope voice lifecycle -- when the source is an envelope modulator, the node reports whether the voice is still active, which integrates with voice management.

**See also:** $SN.core.global_mod$ -- single-source variant without aux modulation, $SN.core.extra_mod$ -- simpler bridge for extra modulation chains
