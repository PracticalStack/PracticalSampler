---
title: Matrix Modulator
moduleId: MatrixModulator
type: Modulator
subtype: EnvelopeModulator
tags: [routing]
builderPath: b.Modulators.MatrixModulator
screenshot: /images/v2/reference/audio-modules/matrixmodulator.png
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors: [number of connections, smoothing active, output range skew]
seeAlso:
  - { id: GlobalModulatorContainer, type: source, reason: "Hosts the source modulators that this matrix reads from" }
  - { id: GlobalEnvelopeModulator, type: alternative, reason: "Reads a single envelope source. Matrix Modulator can combine multiple sources with per-connection modes." }
  - { id: GlobalTimeVariantModulator, type: alternative, reason: "Reads a single time-variant source. Matrix Modulator can combine multiple sources." }
commonMistakes:
  - title: "Requires Global Modulator Container"
    wrong: "Expecting the Matrix Modulator to work without a Global Modulator Container in the module tree"
    right: "Add a Global Modulator Container with source modulators first"
    explanation: "The Matrix Modulator reads from modulators hosted in a Global Modulator Container. Without one, there are no sources to connect to and the modulator outputs only its base value."
  - title: "Scale multiplies, Add offsets"
    wrong: "Mixing up Scale and Add connection modes"
    right: "Scale multiplies against the base value, Add offsets from it"
    explanation: "The combination formula is: output = (baseValue * scaleMod1 * scaleMod2 * ...) + addMod1 + addMod2 + ... Scale connections are applied first (multiplicative), then Add connections (additive)."
  - title: "matrixTargetId without MatrixModulator loses polyphonic modulation"
    wrong: "Using `matrixTargetId` to connect a knob directly to a parameter without a Matrix Modulator in the modulation chain"
    right: "Place a Matrix Modulator in the target's modulation chain and connect the knob to its Value parameter via `matrixTargetId`"
    explanation: "Without a Matrix Modulator in the chain, the value is sent only once per buffer in monophonic mode. The Matrix Modulator provides polyphonic, sample-accurate modulation. [1]($FORUM_REF.13203$)"
  - title: "Call setMatrixModulationProperties at init"
    wrong: "Creating connections without calling `setMatrixModulationProperties` first"
    right: "Call `setMatrixModulationProperties` once in `onInit` before creating connections"
    explanation: "Without this call, mode comboboxes in the matrix table remain empty after drag-and-drop connections are created. [2]($FORUM_REF.13379$)"
forumReferences:
  - id: 1
    title: "matrixTargetId without MatrixModulator loses polyphonic modulation"
    summary: "Using matrixTargetId on a plain knob with no MatrixModulator in the chain sends the value only once per buffer in monophonic mode; the Matrix Modulator is required for polyphonic, sample-accurate modulation."
    topic: 13203
  - id: 2
    title: "setMatrixModulationProperties must be called once at init"
    summary: "Mode comboboxes in the matrix table stay empty after drag-and-drop if setMatrixModulationProperties has not been called; it is a one-time setup call in onInit."
    topic: 13379
  - id: 3
    title: "Use FilterFreq edit-range preset for filter cutoff modulation"
    summary: "When driving filter cutoff from a MatrixModulator, select the FilterFreq or FilterFreq Logarithmic edit-range preset and set the filter frequency parameter to 20 kHz before connecting."
    topic: 13240
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: medium
  description: "Multiple global cables feeding into a multiplication/addition network with smoothed base value"
llmRef: |
  Matrix Modulator (EnvelopeModulator)

  Combines multiple global modulators from a Global Modulator Container into a single modulation output via a connection matrix. Each connection has a mode (Scale/Add/Bipolar), intensity, and optional auxiliary source.

  Signal flow:
    Base value (smoothed) -> multiply by all Scale connections -> add all Add connections -> optional output range skew -> modulation output

  Combination formula: output = (baseValue * scaleMod1 * ... * scaleModN) + addMod1 + ... + addModM

  CPU: medium, scaling with number of connections. Per-sample smoothing of base value plus per-sample processing per connection.

  Parameters:
    Monophonic (Off/On, default dynamic) - skips inner node reset in monophonic mode
    Retrigger (Off/On, default On) - restarts on new note in monophonic mode
    Value (0-100%, default dynamic) - base value before connections are applied
    SmoothingTime (0-2000 ms, default 50 ms) - smoothing time for base value transitions

  Connection modes:
    Scale - multiplicative (multiplies against base value)
    Add - additive (offsets from base value)
    Bipolar - additive with bipolar range display

  When to use:
    When you need to combine multiple global modulators into a single modulation output with individual per-connection modes and intensities. More flexible than single-source Global*Modulator consumers.

  UI: ModulationMatrixPanel and ModulationMatrixControlPanel FloatingTiles.

  Macro controls:
    Assign the same matrixTargetId to multiple MatrixModulators and a UI knob to build unified macro controls.
    Without a MatrixModulator in the chain, matrixTargetId sends values only once per buffer (monophonic).
    Call setMatrixModulationProperties once in onInit before creating connections.

  Common mistakes:
    Requires a GlobalModulatorContainer with source modulators.
    Scale and Add modes have different combination order.
    matrixTargetId without a MatrixModulator in the chain loses polyphonic modulation.
    Mode comboboxes stay empty without calling setMatrixModulationProperties at init.

  See also:
    source GlobalModulatorContainer - hosts the source modulators
    alternative GlobalEnvelopeModulator, GlobalTimeVariantModulator - single-source consumers
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Matrix Modulator screenshot](/images/v2/reference/audio-modules/matrixmodulator.png)

The Matrix Modulator combines multiple global modulators from a Global Modulator Container into a single modulation output. Unlike the single-source Global*Modulator consumers, this module provides a connection matrix where each connection has its own mode (Scale, Add, or Bipolar), intensity, and optional auxiliary source.

The base Value parameter sets the starting point. Scale connections multiply against it, then Add connections offset from the result. The smoothing parameter prevents clicks when the base value changes. When all connections are removed, the modulator automatically bypasses itself and outputs the base value directly.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Base value before connections are applied"
      range: "0 - 100%"
      default: "(dynamic)"
    SmoothingTime:
      desc: "Smoothing time for base value transitions"
      range: "0 - 2000 ms"
      default: "50 ms"
  functions:
    smooth:
      desc: "Per-sample interpolation of the base value at the control rate"
    applyScaleMods:
      desc: "Multiplies the buffer by each Scale connection sequentially"
    applyAddMods:
      desc: "Adds each Add/Bipolar connection to the buffer sequentially"
---

```
// Matrix Modulator - per-voice, envelope
// combines multiple global modulators via connection matrix

onNoteOn() {
    // Reset smoothed base value, initialise all connections
}

perVoiceBlock() {
    // Fill buffer with smoothed base value
    for each sample:
        output[sample] = smooth(Value, SmoothingTime)

    // Apply Scale connections (multiplicative)
    applyScaleMods(output)

    // Apply Add connections (additive)
    applyAddMods(output)

    // Result: (baseValue * scale1 * scale2 ...) + add1 + add2 ...
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Base Value
    params:
      - { name: Value, desc: "The base modulation value before any connections are applied. Normalised to 0-1 via the input range. When all connections are removed, this is the only output.", range: "0 - 100%", default: "(dynamic)" }
      - { name: SmoothingTime, desc: "Time in milliseconds for the base value to reach a new target. Prevents clicks on value changes. At 0 ms the base value changes instantly.", range: "0 - 2000 ms", default: "50 ms" }
  - label: Voice Mode
    params:
      - { name: Monophonic, desc: "Enables monophonic mode. In monophonic mode, inner modulation nodes are not reset on new notes to maintain continuous signal.", range: "Off / On", default: "(dynamic)" }
      - { name: Retrigger, desc: "Restarts the modulator when a new note is triggered in monophonic mode.", range: "Off / On", default: "On" }
---
::

### Connection Properties

Each connection in the matrix has:
- A source modulator (selected from the container's gain chain)
- A mode: Scale (multiplicative), Add (additive), or Bipolar (additive with bipolar range display)
- An intensity slider (-1 to 1)
- An optional auxiliary source with its own intensity
- A per-connection inversion toggle

Connections are created via the matrix table editor, drag-and-drop from the container, or the right-click popup menu. Scale connections are always processed before Add connections, regardless of the order they appear in the connection table.

### Macro Controls with matrixTargetId

Assigning the same `matrixTargetId` to several Matrix Modulators and to a UI control (such as a knob) causes that control to drive the Value parameter of all matching modulators simultaneously. This is the recommended way to build unified macro controls across multiple modulation targets. [3]($FORUM_REF.13203$)

When driving filter cutoff from a Matrix Modulator, use the FilterFreq or FilterFreq Logarithmic edit-range preset. Set the filter's frequency parameter to 20 kHz, then connect the UI knob to the Matrix Modulator's Value parameter. The connection wizard sets the knob range and suffixes automatically. [4]($FORUM_REF.13240$)

### CSS Styling

The matrix table columns can be styled using CSS ID selectors: `#sourceindex`, `#targetid`, `#mode`, `#inverted`, and `#plotter`. Each selector applies to both the header cell and every data cell in that column. The plotter column does not support `background` or `color` CSS properties to avoid the CPU overhead of real-time path rendering at 30 fps. [5]($FORUM_REF.13369$)

**See also:** $MODULES.GlobalModulatorContainer$ -- hosts the source modulators that this matrix reads from, $MODULES.GlobalEnvelopeModulator$ -- reads a single envelope source (simpler, lower CPU), $MODULES.GlobalTimeVariantModulator$ -- reads a single time-variant source (simpler, lower CPU)

- **UI Component:** `ModulationMatrixPanel` - displays the connection matrix
- **UI Component:** `ModulationMatrixControlPanel` - provides drag buttons for creating connections
