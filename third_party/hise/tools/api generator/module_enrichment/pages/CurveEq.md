---
title: Parametriq EQ
moduleId: CurveEq
type: Effect
subtype: MasterEffect
tags: [filter, mixing]
builderPath: b.Effects.CurveEq
screenshot: /images/v2/reference/audio-modules/curveeq.png
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors:
    - { parameter: "Band count", impact: "linear", note: "Each additional band adds one stereo biquad filter to the cascade" }
seeAlso:
  - { id: PolyphonicFilter, type: alternative, reason: "Single filter with modulation chain support - better suited when per-voice filtering or modulated cutoff is needed" }
  - { id: HarmonicFilter, type: alternative, reason: "Harmonic series filter bank with fixed harmonic spacing - a different approach to spectral shaping" }
  - { id: "filters.svf_eq", type: scriptnode, reason: "Scriptnode SVF-based parametric EQ" }
forumReferences:
  - id: 1
    title: "Q below 0.1 causes assertion failure"
    summary: "Setting a band's Q to 0 (e.g., from a knob with default value 0) causes a JUCE assertion failure and breaks the EQ; the minimum valid Q value is 0.1."
    topic: 1308
  - id: 2
    title: "HISE_USE_SVF_FOR_CURVE_EQ eliminates zipper noise on automation"
    summary: "Adding HISE_USE_SVF_FOR_CURVE_EQ=1 to Extra Definitions switches all bands from biquad to SVF filters, eliminating zipper noise when frequency, gain, or Q are automated or scripted in real time."
    topic: 5557
  - id: 3
    title: "No built-in callback for DraggableFilterPanel band dragging"
    summary: "There is no onBandChanged callback; use attachToComponentMouseEvents() on the FloatingTile and read band values in drag/click callbacks to sync external UI controls."
    topic: 6379
commonMistakes:
  - title: "Low Pass/High Pass bands ignore gain"
    wrong: "Setting the Gain parameter on a Low Pass or High Pass band and expecting it to change the sound"
    right: "Use Gain only with Low Shelf, High Shelf, and Peak band types"
    explanation: "Low Pass and High Pass filters have no gain parameter in their transfer function. The Gain value is stored but has no effect on these filter types."
  - title: "Band count scales CPU linearly"
    wrong: "Adding many bands without considering CPU cost"
    right: "Keep band count reasonable for the target platform"
    explanation: "Each band adds a stereo biquad filter processed per sample. CPU cost scales linearly with band count. 3-8 bands is typical; 20+ bands may be expensive on constrained platforms."
  - title: "UI knobs with saveInPreset corrupt EQ band state on preset load"
    wrong: "Connecting scripted knobs to EQ band parameters with `saveInPreset` set to true"
    right: "Set `saveInPreset` to false on any knob that drives EQ band parameters"
    explanation: "On preset load, knobs with saveInPreset write their stored values back to the EQ, overriding the correct module state. EQ band state should be owned exclusively by the module via `Engine.addModuleStateToUserPreset()`."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "A chain of scriptnode biquad filter nodes can replicate arbitrary EQ curves with the added benefit of per-node modulation"
llmRef: |
  Parametriq EQ (Effect/MasterEffect)

  A parametric equaliser with an unlimited number of dynamically managed filter bands and an optional FFT spectrum display. Bands are processed in series (cascaded) through the stereo signal. Each band has independent frequency, gain, Q, type, and enable controls.

  Signal flow:
    stereo in -> [64-sample sub-blocks] -> band 1 filter -> band 2 filter -> ... -> band N filter -> (optional FFT write) -> stereo out

  CPU: medium baseline (scales linearly with band count), monophonic.

  Per-band parameters:
    Gain (-18 to +18 dB, default 0 dB) - boost/cut. Only affects Low Shelf, High Shelf, Peak. No effect on LP/HP.
    Freq (20 - 20000 Hz, default 1500 Hz) - centre frequency.
    Q (0.3 - 8.0, default 1.0) - bandwidth/resonance. On LP/HP bands, the curve display responds to Q but the audio does not (one-pole filters).
    Type (Low Pass, High Pass, Low Shelf, High Shelf, Peak; default Peak) - filter algorithm.
    Enabled (On/Off, default On) - bypasses the individual band.

  Band management:
    Bands are dynamic - added/removed via editor UI or scripting.
    Scripting: setAttribute with flat indexing: bandIndex * BandOffset + parameterConstant.
    Constants: Gain (0), Freq (1), Q (2), Enabled (3), Type (4), BandOffset (5).
    exportState()/restoreState() serialise the full band configuration for EQ preset slots.
    Engine.addModuleStateToUserPreset() required to include EQ state in user presets.
    No built-in callback for band dragging - use attachToComponentMouseEvents() on the FloatingTile.

  Filter mode:
    Default: biquad with 280ms parameter smoothing (coefficient update every 64 samples).
    HISE_USE_SVF_FOR_CURVE_EQ=1: switches to SVF filters - eliminates zipper noise on automation, slightly different tonality. Requires recompile.

  Spectrum display:
    Optional FFT analyser, disabled by default. Rendered by DraggableFilterPanel FloatingTile.

  Common mistakes:
    Setting Gain on LP/HP bands has no effect.
    UI knobs with saveInPreset=true on EQ bands corrupt preset loading - use saveInPreset=false.
    CPU scales linearly with band count.

  Custom equivalent:
    scriptnode HardcodedFX: chain of biquad filter nodes with per-node modulation.

  See also:
    alternative PolyphonicFilter - single filter with modulation chains, per-voice capable
    alternative HarmonicFilter - harmonic series filter bank
    [scriptnode] filters.svf_eq - scriptnode SVF-based parametric EQ
---

::category-tags
---
tags:
  - { name: filter, desc: "Modules that shape the frequency spectrum of the signal" }
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance" }
---
::

![Parametriq EQ screenshot](/images/v2/reference/audio-modules/curveeq.png)

The Parametriq EQ is a fully parametric equaliser with a dynamic number of filter bands and an optional FFT spectrum display. Unlike most HISE modules, it has no fixed parameters - bands are added and removed at runtime, each with independent control over frequency, gain, Q factor, filter type, and enable state.

All bands are processed in series: each band's output feeds the next, forming a cascaded filter chain. The order of bands matters, though in practice the audible difference from reordering is negligible for most EQ configurations. An optional spectrum analyser overlays the post-filter frequency content behind the filter curve display, providing real-time visual feedback.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Per-band boost/cut in decibels. Only affects shelving and peak types."
      range: "-18 - +18 dB"
      default: "0 dB"
    Freq:
      desc: "Per-band centre frequency"
      range: "20 - 20000 Hz"
      default: "1500 Hz"
    Q:
      desc: "Per-band bandwidth / resonance"
      range: "0.3 - 8.0"
      default: "1.0"
    Type:
      desc: "Per-band filter algorithm"
      range: "Low Pass, High Pass, Low Shelf, High Shelf, Peak"
      default: "Peak"
    Enabled:
      desc: "Per-band bypass toggle"
      range: "On / Off"
      default: "On"
  functions:
    applyBiquad:
      desc: "Applies a stereo biquad filter with 280ms parameter smoothing, recalculating coefficients every 64 samples"
    writeFFT:
      desc: "Writes the post-filter output to the FFT ring buffer for spectrum display"
---

```
// Parametriq EQ - monophonic, series cascade
// stereo in -> stereo out (in-place)

process(left, right) {
    // Process in 64-sample sub-blocks for smooth coefficient updates
    for each 64-sample chunk {

        for each band {
            if Enabled:
                applyBiquad(chunk, Freq, Gain, Q, Type)
                // 280ms smoothing on frequency, gain, and Q changes
        }
    }

    // Optional spectrum analyser (post-filter)
    if fftEnabled:
        writeFFT(output)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Per-Band Controls
    params:
      - { name: Gain, desc: "Boost or cut in decibels. Only applies to Low Shelf, High Shelf, and Peak filter types. Has no effect on Low Pass or High Pass bands.", range: "-18 - +18 dB", default: "0 dB" }
      - { name: Freq, desc: "Centre frequency of the filter band. Uses a skewed distribution centred at 1500 Hz for more natural control feel.", range: "20 - 20000 Hz", default: "1500 Hz" }
      - name: Q
        desc: "Bandwidth of the filter. Lower values produce a wider curve; higher values produce a narrower, more resonant peak."
        range: "0.3 - 8.0"
        default: "1.0"
        hints:
          - type: warning
            text: "The minimum valid Q value is 0.1. Setting Q to 0 (e.g., from a UI knob whose default is 0) triggers a JUCE assertion failure and breaks the EQ. Ensure all Q controls have a minimum of 0.1."
          - type: warning
            text: "On **Low Pass** and **High Pass** bands, the filter curve display responds to Q changes but the audio output does not. These are one-pole filters with no resonance parameter. Enable `HISE_USE_SVF_FOR_CURVE_EQ` for SVF-based LP/HP where Q shapes the actual response."
      - { name: Type, desc: "The filter algorithm for this band.", range: "Low Pass, High Pass, Low Shelf, High Shelf, Peak", default: "Peak" }
      - { name: Enabled, desc: "Toggles the band on or off. Disabled bands are skipped during processing at zero cost.", range: "On / Off", default: "On" }
---
::

### Band Management

Bands are managed dynamically. In the editor, double-click the filter curve display to add a band, or right-click an existing band to remove it. From HISEScript, use the `setDraggableFilterData()` and `getDraggableFilterData()` methods on the effect reference, or address individual band parameters with flat indexing:

```
// Example: set band 0's frequency to 2000 Hz
eq.setAttribute(0 * eq.BandOffset + eq.Freq, 2000.0);

// Example: set band 2's type to High Shelf (3)
eq.setAttribute(2 * eq.BandOffset + eq.Type, 3);
```

The available scripting constants are: `Gain` (0), `Freq` (1), `Q` (2), `Enabled` (3), `Type` (4), and `BandOffset` (5). The minimum valid Q value is 0.1 — passing 0 triggers a JUCE assertion failure that breaks the EQ. Ensure any UI knob or script driving Q has a lower bound of at least 0.1. [1]($FORUM_REF.1308$)

The complete band configuration can be serialised with `exportState()` and restored with `restoreState()`. This is the recommended approach for implementing named EQ preset slots (e.g., stored in a ComboBox or array of JSON objects). To include EQ band state in user presets, call `Engine.addModuleStateToUserPreset()` with the EQ module reference - the DraggableFilterPanel FloatingTile does not participate in user preset saving by default.

There is no built-in callback that fires when a user drags a band in the DraggableFilterPanel. To sync external UI controls (e.g., knobs showing a band's frequency or gain), attach a broadcaster to the FloatingTile's mouse events via `attachToComponentMouseEvents()` and read band values on drag and click events. [3]($FORUM_REF.6379$)

### Smoothing and Filter Mode

Each band uses biquad filters by default with 280ms parameter smoothing. Coefficients are updated every 64 samples, so rapid automation produces smooth transitions without audible stepping. There are no modulation chains - all parameter changes are applied directly.

Adding `HISE_USE_SVF_FOR_CURVE_EQ=1` to the project's Extra Definitions switches all bands from biquad to state-variable filters. SVF filters do not produce zipper noise when frequency, gain, or Q are changed in real time, making them preferable when bands will be automated or scripted to move. The tonal character differs slightly from the default biquad mode. This setting requires recompiling the plugin. [2]($FORUM_REF.5557$)

### Spectrum Analyser

The spectrum analyser is disabled by default to save CPU. It shows the post-filter output spectrum overlaid behind the filter curve. The analyser can be toggled from the editor toolbar.

### DraggableFilterPanel

The DraggableFilterPanel FloatingTile embeds the EQ display in custom interfaces. Its visual rendering is fully overridable via a local LookAndFeel object - use a local (not global) LAF to avoid issues with compile error recovery.

The module is automatically suspended when the input signal is silent.

**See also:** $MODULES.PolyphonicFilter$ -- Single filter with modulation chain support - better suited when per-voice filtering or modulated cutoff is needed, $MODULES.HarmonicFilter$ -- Harmonic series filter bank with fixed harmonic spacing - a different approach to spectral shaping, $SN.filters.svf_eq$ -- scriptnode SVF-based parametric EQ
