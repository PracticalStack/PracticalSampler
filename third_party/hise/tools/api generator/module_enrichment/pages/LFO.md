---
title: LFO Modulator
moduleId: LFO
type: Modulator
subtype: TimeVariantModulator
tags: [generator, oscillator]
builderPath: b.Modulators.LFO
screenshot: /images/v2/reference/audio-modules/lfo.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "LoopEnabled has no effect on standard waveforms"
    wrong: "Setting LoopEnabled to Off and expecting Sine/Triangle/Saw/Square to play once"
    right: "LoopEnabled only affects Custom and Steps modes; standard waveforms always loop"
    explanation: "Use the Custom waveform with a drawn shape and LoopEnabled Off for one-shot LFO behaviour with standard shapes."
  - title: "SyncToMasterClock requires TempoSync"
    wrong: "Enabling SyncToMasterClock without also enabling TempoSync"
    right: "SyncToMasterClock requires TempoSync to be on; it has no effect when the LFO runs in free Hz mode"
    explanation: "Master clock sync aligns the LFO phase to the host transport, which only makes sense when the LFO rate is defined as a tempo division."
  - title: "FrequencyChain is heavily downsampled"
    wrong: "Expecting per-sample accuracy from frequency modulation via the FrequencyChain"
    right: "The frequency chain is heavily downsampled and only updates the LFO rate every ~4096 audio samples"
    explanation: "For precise FM-style modulation, use scriptnode instead. The LFO's frequency modulation is designed for slow, smooth rate changes."
  - title: "SmoothingTime blurs step transitions"
    wrong: "Using high SmoothingTime values with the Steps waveform and expecting sharp step transitions"
    right: "Reduce SmoothingTime to 0 for crisp step transitions"
    explanation: "The smoothing filter softens all waveform output including steps. High values blur the step boundaries."
  - title: "FadeIn fails in Container modulation chains"
    wrong: "Placing an LFO in a Container's modulation chain and expecting FadeIn to work"
    right: "Place the LFO inside the sound generator's modulation chain or use a Global LFO Modulator"
    explanation: "Containers do not process note-on messages, so the LFO never resets and FadeIn never triggers."
forumReferences:
  - id: 1
    title: "Control rate processing degrades LFO accuracy above ~30 Hz"
    summary: "All HISE modulators run at 1/8th audio rate by default; this aliases the LFO above ~30 Hz but can be improved by setting HISE_EVENT_RASTER to 4, 2, or 1 in Extra Definitions."
    topic: 3639
  - id: 2
    title: "LFO pitch center shifts when intensity is reduced"
    summary: "Reducing the intensity of an LFO assigned to pitch does not return the pitch to its unmodulated baseline — the pitch center shifts asymmetrically."
    topic: 4674
  - id: 3
    title: "TempoSync maximum is 1/1 bar; slower rates require C++ source edit"
    summary: "The built-in TempoSync range tops out at 1/1 bar; values like 2/1 or 4/1 require appending new entries to the TempoSyncer enum in MiscToolClasses.cpp/.h."
    topic: 6446
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedTimeVariantModulator
  complexity: medium
  description: "Build with an oscillator node, envelope node for fade-in, and a tempo sync node for clock-locked rate control"
llmRef: |
  LFO Modulator (Modulator/TimeVariantModulator)

  Generates a periodic modulation signal with selectable waveform, tempo sync, fade-in envelope, and step sequencer mode. Monophonic - all voices share the same output.

  Signal flow:
    phase accumulator -> waveform lookup -> fade-in envelope -> mode mapping -> smoothing -> intensity scaling -> modulation out

  CPU: low, monophonic (runs at control rate, not audio rate)

  Parameters:
    Frequency (0.01-40 Hz or tempo division, default 3 Hz) - LFO rate, modulatable via FrequencyChain
    FadeIn (0-3000 ms, default 1000 ms) - exponential fade-in after note trigger
    WaveformType (Sine/Triangle/Saw/Square/Random/Custom/Steps, default Sine) - selects waveform source
    Legato (On/Off, default On) - prevents phase reset on overlapping notes
    TempoSync (On/Off, default Off) - switches Frequency to tempo divisions
    SmoothingTime (0-1000 ms, default 5 ms) - output smoothing to reduce discontinuities
    NumSteps (1-128, default 16) - number of steps in Steps mode
    LoopEnabled (On/Off, default On) - enables looping (Custom/Steps only; standard waveforms always loop)
    PhaseOffset (0-100%, default 0%) - initial phase offset on reset
    SyncToMasterClock (On/Off, default Off) - aligns phase to host transport (requires TempoSync)
    IgnoreNoteOn (On/Off, default Off) - free-run mode, no note-triggered phase reset

  Modulation chains:
    LFO Intensity Mod - scales the output depth (post-multiply)
    LFO Frequency Mod - scales the LFO rate (linked to Frequency parameter)

  When to use:
    Standard periodic modulation for tremolo, vibrato, filter sweeps, pan movement. Use Custom waveform for arbitrary shapes. Use Steps mode for rhythmic step sequencer patterns. Enable TempoSync for beat-locked modulation.

  Common mistakes:
    LoopEnabled Off has no effect on Sine/Triangle/Saw/Square - only Custom and Steps.
    SyncToMasterClock requires TempoSync to be enabled.
    FrequencyChain is heavily downsampled - not suitable for precise FM.
    High SmoothingTime blurs step transitions in Steps mode.
    FadeIn has no effect when LFO is in a Container's modulation chain - use Global LFO or place inside the sound generator.
    getCurrentLevel() returns 0 in compiled plugins unless ENABLE_ALL_PEAK_METERS=1 is set.
    LFO accuracy degrades above ~30 Hz due to control rate (adjustable via HISE_EVENT_RASTER).
    Only SVF and Ladder filters handle LFO modulation smoothly - biquad types produce zipper noise.

  Custom equivalent:
    scriptnode via HardcodedTimeVariantModulator: oscillator + envelope + tempo sync nodes.

  See also: none
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
  - { name: oscillator, desc: "Modules that generate audio or modulation signals from oscillators or synthesis algorithms" }
---
::

![LFO Modulator screenshot](/images/v2/reference/audio-modules/lfo.png)

The LFO Modulator generates a periodic modulation signal from one of seven waveform types: Sine, Triangle, Saw, Square, Random, Custom (user-drawn table), or Steps (step sequencer). It is monophonic - all voices share the same LFO output, making it suitable for global modulation effects like tremolo, vibrato, and filter sweeps.

The LFO rate can run freely in Hz or lock to the host tempo. A fade-in envelope ramps the modulation depth after each note trigger, preventing abrupt modulation jumps. Two modulation chains allow external control of the output intensity and the LFO frequency. The output is smoothed to reduce discontinuities, particularly useful for the Random and Steps waveforms.

The LFO output semantics change depending on where it is placed in the module tree. In a gain chain, the output modulates downward from 1.0. In a pitch or pan chain, the output can be bipolar (centred at 0.5) or unipolar depending on the bipolar setting. This is handled automatically - no user configuration is needed.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "LFO rate in Hz or as a tempo-synced note division"
      range: "0.01 - 40 Hz"
      default: "3 Hz"
    WaveformType:
      desc: "Selects the waveform: Sine, Triangle, Saw, Square, Random, Custom, or Steps"
      range: "Sine, Triangle, Saw, Square, Random, Custom, Steps"
      default: "Sine"
    FadeIn:
      desc: "Fade-in time after note trigger"
      range: "0 - 3000 ms"
      default: "1000 ms"
    SmoothingTime:
      desc: "Output smoothing to reduce discontinuities"
      range: "0 - 1000 ms"
      default: "5 ms"
    TempoSync:
      desc: "Switches Frequency from Hz to tempo-synced note divisions"
      range: "Off / On"
      default: "Off"
    NumSteps:
      desc: "Number of steps in the step sequencer (Steps mode only)"
      range: "1 - 128"
      default: "16"
    LoopEnabled:
      desc: "Enables waveform looping (only affects Custom and Steps modes)"
      range: "Off / On"
      default: "On"
    Legato:
      desc: "Prevents phase reset when overlapping notes are played"
      range: "Off / On"
      default: "On"
    IgnoreNoteOn:
      desc: "Free-run mode - LFO phase is not reset by note events"
      range: "Off / On"
      default: "Off"
    PhaseOffset:
      desc: "Initial phase offset applied when the LFO resets"
      range: "0 - 100%"
      default: "0%"
    SyncToMasterClock:
      desc: "Aligns the LFO phase to the host transport position (requires TempoSync)"
      range: "Off / On"
      default: "Off"
  functions:
    waveformLookup:
      desc: "Reads the current waveform value from a lookup table with linear interpolation"
    fadeInEnvelope:
      desc: "Exponential fade-in that ramps from zero to full intensity over the FadeIn time"
    smooth:
      desc: "Low-pass smoothing filter applied to the output"
  modulations:
    LFOIntensityMod:
      desc: "Scales the LFO output depth"
      scope: "monophonic"
    LFOFrequencyMod:
      desc: "Scales the LFO frequency"
      scope: "monophonic"
---

```
// LFO Modulator - periodic modulation signal
// monophonic, runs at control rate

// Frequency control
rate = TempoSync ? tempoToHz(Frequency, HostBPM) : Frequency
rate = rate * LFOFrequencyMod

// Per control-rate sample
value = waveformLookup(phase, WaveformType)
value = fadeInEnvelope(value, FadeIn)
value = smooth(value, SmoothingTime)

// Post-processing
output = value * LFOIntensityMod

phase += rate    // advance phase
```

::

## Parameters

::parameter-table
---
groups:
  - label: Oscillator
    params:
      - name: Frequency
        desc: "LFO rate. In free mode, sets the frequency in Hz. With TempoSync enabled, selects a tempo-synced note division. Modulatable via the Frequency modulation chain."
        range: "0.01 - 40 Hz"
        default: "(dynamic)"
        hints:
          - type: warning
            text: "Accuracy degrades above ~30 Hz due to control-rate processing (`HISE_EVENT_RASTER`). Set to `4`, `2`, or `1` in **Extra Definitions** to improve. [1]($FORUM_REF.3639$)"
      - { name: WaveformType, desc: "Selects the waveform shape. See the waveform table below for visual reference. Sine, Triangle, Saw, and Square use pre-computed lookup tables. Random generates a new value each cycle. Custom uses a user-drawn table curve. Steps reads from a slider pack step sequencer.", range: "Sine, Triangle, Saw, Square, Random, Custom, Steps", default: "Sine" }
      - { name: PhaseOffset, desc: "Initial phase offset applied when the LFO resets on note trigger. Does not affect the waveform shape, only the starting position.", range: "0 - 100%", default: "0%" }
  - label: Tempo & Sync
    params:
      - name: TempoSync
        desc: "Switches the Frequency parameter from Hz to tempo-synced note divisions (e.g. Quarter, Eighth, Sixteenth)"
        range: "Off / On"
        default: "Off"
        hints:
          - type: tip
            text: "Supports divisions from 1/32T up to 1/1 bar. For slower rates (2/1, 4/1), use a scriptnode LFO with a tempo node that supports a multiplier parameter. [3]($FORUM_REF.6446$)"
      - name: SyncToMasterClock
        desc: "Aligns the LFO phase to the host transport position on play start and resync events. Only active when TempoSync is also enabled."
        range: "Off / On"
        default: "Off"
        hints:
          - type: tip
            text: "For precise beat-grid alignment including mid-bar starts, combine with `Engine.createTransportHandler()` using `setEnableGrid(true, 8)` and `setSyncMode(PreferExternal)`."
  - label: Triggering
    params:
      - { name: Legato, desc: "When enabled, the LFO does not reset its phase when a new note is played while other notes are held. Only the first note triggers a reset.", range: "Off / On", default: "On" }
      - { name: IgnoreNoteOn, desc: "Free-run mode. The LFO runs continuously without resetting on note events. Useful for global modulation effects that should not be tied to note timing.", range: "Off / On", default: "Off" }
  - label: Envelope
    params:
      - name: FadeIn
        desc: "Fade-in time after each note trigger. The LFO output ramps from zero to full depth over this period using an exponential curve. Set to 0 for instant full depth."
        range: "0 - 3000 ms"
        default: "1000 ms"
        hints:
          - type: warning
            text: "Has no effect when the LFO is in a Container's modulation chain. Place inside the sound generator or use a Global LFO."
  - label: Output
    params:
      - { name: SmoothingTime, desc: "Smoothing time applied to the output signal. Reduces discontinuities in waveforms like Random and Steps. Set to 0 for unsmoothed output.", range: "0 - 1000 ms", default: "5 ms" }
      - { name: LoopEnabled, desc: "Enables waveform looping. When disabled, Custom and Steps waveforms play once and hold their final value. Standard waveforms (Sine, Triangle, Saw, Square) always loop regardless of this setting.", range: "Off / On", default: "On" }
  - label: Step Sequencer
    params:
      - { name: NumSteps, desc: "Number of active steps in the step sequencer. Only used when WaveformType is set to Steps.", range: "1 - 128", default: "16" }
---
::

### Waveform Types

| Waveform | Shape | Description |
|----------|-------|-------------|
| Sine | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><path d="M0,16 C10,0 20,0 30,16 C40,32 50,32 60,16 C65,8 67.5,4 70,4" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | Smooth periodic waveform. Default shape. |
| Triangle | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><polyline points="0,16 20,2 40,30 60,2 80,30" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | Linear ramp up and down. |
| Saw | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><polyline points="0,30 40,2 40,30 80,2" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | Linear ramp up, instant reset. |
| Square | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><polyline points="0,30 0,2 20,2 20,30 40,30 40,2 60,2 60,30 80,30" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | Alternates between minimum and maximum. |
| Random | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><polyline points="0,20 10,20 10,8 20,8 20,26 30,26 30,12 40,12 40,22 50,22 50,6 60,6 60,28 70,28 70,14 80,14" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | New random value each cycle. |
| Custom | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><path d="M0,28 Q10,28 15,20 T30,8 Q40,4 50,14 T65,24 Q72,28 80,20" fill="none" stroke="currentColor" stroke-width="1.5"/><line x1="0" y1="30" x2="80" y2="30" stroke="currentColor" stroke-width="0.5" stroke-dasharray="2,2"/></svg> | User-drawn table shape. Supports one-shot playback via LoopEnabled. |
| Steps | <svg viewBox="0 0 80 32" width="80" height="32" xmlns="http://www.w3.org/2000/svg"><polyline points="0,24 10,24 10,8 20,8 20,18 30,18 30,4 40,4 40,28 50,28 50,12 60,12 60,20 70,20 70,6 80,6" fill="none" stroke="currentColor" stroke-width="1.5"/></svg> | Step sequencer driven by a slider pack. Slider values are inverted so that sliders pushed up produce high modulation values. There is a brief crossfade at each step transition to avoid clicks. NumSteps controls step count. |

## Modulation Chains

::modulation-table
---
chains:
  - name: "LFO Intensity Mod"
    desc: "Scales the LFO output depth. Applied as a post-multiply after all per-sample processing."
    scope: "monophonic"
    constrainer: "Any"
    hints:
      - type: warning
        text: "In **pitch mode**, reducing intensity shifts the pitch center asymmetrically. Add a `Constant` modulator to offset, or use a scriptnode LFO with explicit bipolar output. [2]($FORUM_REF.4674$)"
  - { name: "LFO Frequency Mod", desc: "Scales the LFO frequency. Multiplies the base rate (Hz or tempo-derived). Updated approximately every 4096 audio samples, not per-sample.", scope: "monophonic", constrainer: "Any" }
---
::

### Scripting Access

When using the Custom waveform, `Synth.getTableProcessor("LFO1")` and `Synth.getModulator("LFO1")` return different typed references to the same module. Create both if you need to control the table shape and the modulator parameters from script.
