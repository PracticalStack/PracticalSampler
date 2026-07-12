---
title: Script Time Variant Modulator
moduleId: ScriptTimeVariantModulator
type: Modulator
subtype: TimeVariantModulator
tags: [custom]
builderPath: b.Modulators.ScriptTimeVariantModulator
screenshot: /images/v2/reference/audio-modules/scripttimevariantmodulator.png
cpuProfile:
  baseline: "(depends on loaded network)"
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: HardcodedTimevariantModulator, type: alternative, reason: "Compiled variant for exported plugins" }
  - { id: LFO, type: alternative, reason: "Built-in LFO when a standard waveform shape is sufficient" }
commonMistakes:
  - title: "Network output exceeds 0-1 range"
    wrong: "Expecting modulation values outside the 0-1 range to pass through"
    right: "Design the network to produce output in the 0-1 range - values are clipped automatically in network mode"
    explanation: "In network mode, the output is hard-clipped to 0.0-1.0. Values outside this range are clamped. In HISEScript mode, no automatic clipping is applied."
  - title: "Network not compiled before export"
    wrong: "Exporting with an uncompiled network"
    right: "Compile the network to a DLL before exporting, or switch to HardcodedTimeVariantModulator"
    explanation: "Uncompiled networks only work in the HISE IDE."
llmRef: |
  Script Time Variant Modulator (Modulator/TimeVariantModulator)

  Monophonic continuous modulator that generates a modulation signal from a scriptnode DSP network or HISEScript callbacks. Output is clipped to 0-1 in network mode.

  Signal flow:
    [network loaded?] -> network process (1 channel) -> clip 0-1 -> modulation out
                      -> script processBlock -> modulation out

  CPU: depends on loaded network, monophonic

  Parameters:
    No fixed parameters (offset = 0). All parameters come from the loaded network.

  Modulation chains:
    None. This module IS a modulator - it does not have parent Gain/Pitch chains or extra modulation slots.

  Channel configuration:
    Not applicable - produces a single-channel modulation signal, not audio output.

  Complex data types and parameter exposure:
    No fixed parameters (offset = 0). See Audio Modules index Custom section for complex data types, parameter exposure, and configuration table.

  When to use:
    Use for custom modulation sources like complex LFO shapes, step sequencers, or algorithmic modulators that cannot be built with the standard LFO or ConstantModulator.

  Common mistakes:
    Network output is clipped to 0-1 in network mode.
    Must compile network before export.

  See also:
    alternative HardcodedTimevariantModulator - compiled variant
    alternative LFO - built-in LFO for standard shapes
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules with user-defined signal paths via scriptnode networks or HISEScript callbacks" }
---
::

![Script Time Variant Modulator screenshot](/images/v2/reference/audio-modules/scripttimevariantmodulator.png)

The Script Time Variant Modulator generates a continuous monophonic modulation signal from either a scriptnode DSP network or HISEScript callbacks. It produces a single-channel output clipped to the 0-1 range, suitable for modulating parameters on any module in the tree. Use this when you need a custom modulation source that goes beyond what the built-in [LFO]($MODULES.LFO$) or other standard modulators can provide - for example, complex waveshaping, step sequences, or algorithmic patterns.

The module supports two operating modes. When a scriptnode network is loaded, it processes through the network and clips the output to 0-1. When no network is loaded, it falls back to HISEScript callbacks (`prepareToPlay`, `processBlock`, `onNoteOn`, `onNoteOff`, `onController`) for maximum flexibility. The network mode is preferred for performance-critical applications.

## Signal Path

::signal-path
---
glossary:
  functions:
    networkProcess:
      desc: "Processes the modulation signal through the scriptnode network (1 channel)"
    clipOutput:
      desc: "Clamps the output to the valid 0.0-1.0 modulation range"
    scriptProcess:
      desc: "Fallback: processes through HISEScript processBlock callback"
---

```
// Script Time Variant Modulator - continuous monophonic modulator
// -> modulation out (0-1)

calculateBlock() {
    if (networkLoaded) {
        networkProcess(buffer)
        clipOutput(buffer, 0.0, 1.0)
    } else {
        scriptProcess(buffer)
    }
}
```

::

### Loading a Network

Create a scriptnode network in the `onInit` callback by calling `Engine.createDspNetwork("NetworkName")`. This creates a new network or loads an existing one from the `DspNetworks/Networks/` folder, where networks are stored as `.xml` files.

The network should produce a single-channel modulation signal. It operates in monophonic mode — there is no per-voice state. The output is processed at audio rate and hard-clipped to the 0-1 range automatically.

Switching networks at runtime is possible by calling `Engine.createDspNetwork()` again with a different name. However, this is a heavyweight operation that reinitialises the entire DSP graph and should not be done during audio playback. For preset-style switching, expose network parameters and change those instead.

### Modulation Chain Configuration

Not applicable. This module **is** a modulator — it does not have parent Gain or Pitch chains, nor extra modulation chain slots. All modulation within the network must be handled internally using `container.modchain`, modulation cables, and `control.*` nodes.

### Parameter Exposure and Complex Data

This module has no fixed parameters — all network parameters start at index 0. The `Intensity` parameter name is reserved (it is a built-in modulator property) and cannot be used as a network parameter name. See [Custom module hosting](/v2/reference/audio-modules/#custom) for parameter exposure, complex data types, and the configuration table.

### HISEScript Mode

When no network is loaded, the module offers seven HISEScript callbacks:

- `onInit` - initialisation
- `prepareToPlay` - called when sample rate or block size changes
- `processBlock` - main processing callback, receives a buffer to fill with modulation values
- `onNoteOn` / `onNoteOff` - MIDI note events
- `onController` - MIDI CC events
- `onControl` - UI control changes

This mode does not clip the output automatically - the script is responsible for producing values in the 0-1 range.

**See also:** $MODULES.HardcodedTimevariantModulator$ -- compiled variant for exported plugins, $MODULES.LFO$ -- built-in LFO for standard waveform shapes
