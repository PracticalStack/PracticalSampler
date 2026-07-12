---
title: Filter
moduleId: PolyphonicFilter
type: Effect
subtype: VoiceEffect
tags: [filter]
builderPath: b.Effects.PolyphonicFilter
screenshot: /images/v2/reference/audio-modules/polyphonicfilter.png
cpuProfile:
  baseline: medium
  polyphonic: true
  scalingFactors:
    - "Cost scales linearly with active voice count"
    - "MoogLP and LadderFourPoleLP modes are more expensive than biquad or one-pole modes"
seeAlso:
  - { id: CurveEq, type: alternative, reason: "Monophonic multi-band parametric EQ with visual editor - use when you need multiple filter bands on the master signal rather than per-voice filtering" }
  - { id: HarmonicFilter, type: alternative, reason: "Polyphonic filter bank tuned to the harmonic series - use for harmonic-aware spectral shaping rather than a single cutoff frequency" }
  - { id: "filters.svf", type: scriptnode, reason: "Scriptnode state variable filter with superior modulation stability" }
  - { id: "filters.biquad", type: scriptnode, reason: "Scriptnode biquad filter with six modes" }
  - { id: "filters.ladder", type: scriptnode, reason: "Scriptnode 24 dB/oct ladder filter" }
  - { id: "filters.moog", type: scriptnode, reason: "Scriptnode Moog-style transistor ladder filter" }
commonMistakes:
  - title: "Gain only affects shelves and peaks"
    wrong: "Setting Gain and expecting it to affect a low-pass or high-pass filter mode"
    right: "The Gain parameter only affects LowShelf, HighShelf, and Peak modes"
    explanation: "For all other filter types, the Gain parameter has no audible effect. Switch to a shelf or peak mode before adjusting Gain."
  - title: "Use CurveEq for master bus"
    wrong: "Adding the module to shape the master output signal"
    right: "Use CurveEq or a monophonic effect for master bus filtering"
    explanation: "PolyphonicFilter is a per-voice effect. When no polyphonic modulators are connected it falls back to monophonic processing, but CurveEq is designed for that purpose and offers multiple bands."
  - title: "StateVariablePeak and LadderFourPoleHP broken"
    wrong: "Selecting StateVariablePeak or LadderFourPoleHP mode and expecting the filter to change"
    right: "These two modes are non-functional - the previously selected filter type remains active"
    explanation: "StateVariablePeak (index 11) and LadderFourPoleHP (index 16) are listed in the mode selector but do not switch the filter. Avoid these modes."
forumReferences:
  - id: 1
    title: "Frequency modulation scales to knob value, not around it"
    summary: "The Frequency Modulation chain multiplies the normalised frequency by 0-1, sweeping from ~20 Hz up to the knob value rather than modulating around a centre frequency. Use the Bipolar Freq Modulation chain to offset from a centre."
    topic: 2749
  - id: 2
    title: "Any polyphonic modulator forces per-voice processing"
    summary: "Adding even one polyphonic modulator to any chain switches the entire filter to per-voice processing, significantly raising CPU cost compared to the monophonic fallback path."
    topic: 404
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "A scriptnode network with a polyphonic filter node (e.g. svf, biquad, or ladder) and modulation connections replicates the core behaviour. The bipolar frequency modulation path requires additional math nodes."
llmRef: |
  PolyphonicFilter (VoiceEffect)

  Per-voice filter with 18 selectable modes (LP, HP, shelf, peak, SVF, Moog, ladder, allpass, ring mod, one-pole). Automatically switches between per-voice and monophonic processing based on whether any modulation chain contains polyphonic modulators. Default: StateVariableLP at 20000 Hz (transparent).

  Signal flow:
    audio in (per-voice) -> combine frequency modulation (normalise + bipolar offset + freq mod scale) -> update coefficients if changed -> filter process (topology selected by Mode) -> audio out

  CPU: medium baseline, scales linearly with voice count. MoogLP and Ladder modes are more expensive.

  Parameters:
    Frequency (20-20000 Hz, default 20000 Hz) - cutoff/centre frequency. Default passes all audible frequencies.
    Q (0.3-8.0, default 1.0) - resonance/bandwidth.
    Gain (-18 to 18 dB, default 0 dB) - only affects LowShelf, HighShelf, and Peak modes.
    Mode (0-17, default 6/StateVariableLP) - selects filter type. Modes 11 (StateVariablePeak) and 16 (LadderFourPoleHP) are non-functional.
    Quality (0-4096, default 512) - has no effect on processing.
    BipolarIntensity (-1.0 to 1.0, default 0.0) - scales the bipolar frequency modulation offset.

  Modulation chains:
    Frequency Modulation (gain mode) - scales normalised cutoff frequency. Per-voice when polyphonic modulators present.
    Gain Modulation (gain mode) - scales filter gain for shelf/peak modes.
    Bipolar Freq Modulation (offset mode) - adds signed offset to normalised frequency before standard frequency modulation.
    Q Modulation (gain mode) - scales the Q value.

  Filter modes:
    0=LowPass, 1=HighPass, 2=LowShelf, 3=HighShelf, 4=Peak, 5=ResoLow, 6=StateVariableLP, 7=StateVariableHP, 8=MoogLP, 9=OnePoleLowPass, 10=OnePoleHighPass, 11=StateVariablePeak(broken), 12=StateVariableNotch, 13=StateVariableBandPass, 14=Allpass, 15=LadderFourPoleLP, 16=LadderFourPoleHP(broken), 17=RingMod

  When to use:
    Per-voice filtering in polyphonic instruments. Connect an envelope to the Frequency Modulation chain for classic filter sweeps per voice.

  Common mistakes:
    Gain parameter only works with shelf/peak modes.
    StateVariablePeak and LadderFourPoleHP modes are non-functional.
    Quality parameter has no effect.

  See also:
    CurveEq - monophonic multi-band parametric EQ alternative
    HarmonicFilter - polyphonic harmonic series filter
    [scriptnode] filters.svf - scriptnode state variable filter with superior modulation stability
    [scriptnode] filters.biquad - scriptnode biquad filter with six modes
    [scriptnode] filters.ladder - scriptnode 24 dB/oct ladder filter
    [scriptnode] filters.moog - scriptnode Moog-style transistor ladder filter
---

::category-tags
---
tags:
  - { name: filter, desc: "Effects that shape the frequency spectrum of the audio signal" }
---
::

![Filter screenshot](/images/v2/reference/audio-modules/polyphonicfilter.png)

A per-voice filter effect with 18 selectable filter types including low-pass, high-pass, shelving, peak, state variable, Moog ladder, and ring modulation modes. Each active voice runs its own independent filter instance, making it suitable for classic polyphonic filter sweeps driven by envelopes or LFOs.

The module automatically switches between per-voice and monophonic processing. When any modulation chain contains a polyphonic modulator, audio is filtered independently per voice. When all modulators are monophonic, the summed signal is processed as a single stream with coefficient updates every 64 samples. The default settings (StateVariableLP at 20000 Hz) pass all audible frequencies, so the filter is effectively transparent until you lower the Frequency or change the Mode.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Cutoff or centre frequency of the filter"
      range: "20 - 20000 Hz"
      default: "20000 Hz"
    Q:
      desc: "Resonance or bandwidth of the filter"
      range: "0.3 - 8.0"
      default: "1.0"
    Gain:
      desc: "Filter gain in dB for shelf and peak modes only"
      range: "-18 - 18 dB"
      default: "0 dB"
    Mode:
      desc: "Selects the filter type (18 modes)"
      range: "0 - 17"
      default: "6 (StateVariableLP)"
    BipolarIntensity:
      desc: "Scales the bipolar frequency modulation offset"
      range: "-1.0 - 1.0"
      default: "0.0"
  functions:
    normalise:
      desc: "Maps frequency from Hz (20-20000) to a 0-1 range for modulation"
    denormalise:
      desc: "Maps normalised frequency back to Hz (20-20000)"
    updateCoefficients:
      desc: "Recalculates filter coefficients when frequency, gain, or Q have changed"
    filterProcess:
      desc: "Applies the selected filter topology to the audio buffer in-place, per sample"
  modulations:
    FrequencyModulation:
      desc: "Scales the normalised cutoff frequency (gain mode, 0-1)"
      scope: "per-voice"
    BipolarFreqModulation:
      desc: "Adds a signed offset to the normalised frequency (offset mode, -1 to +1)"
      scope: "per-voice"
    GainModulation:
      desc: "Scales filter gain for shelf and peak modes"
      scope: "per-voice"
    QModulation:
      desc: "Scales the Q value"
      scope: "per-voice"
---

```
// Filter - per-voice polyphonic filter
// audio in (per-voice) -> audio out (per-voice)

process(left, right) {
    // Frequency modulation: normalise, apply bipolar offset, then scale
    normFreq = normalise(Frequency)
    normFreq = normFreq + BipolarIntensity * BipolarFreqModulation
    normFreq = normFreq * FrequencyModulation
    freq = denormalise(normFreq)

    // Apply gain and Q modulation
    q = Q * QModulation
    gain = Gain * GainModulation    // shelf and peak modes only

    // Recalculate coefficients only when values have changed
    updateCoefficients(Mode, freq, q, gain)

    // Apply filter in-place (topology depends on Mode)
    filterProcess(left, right)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Filter Configuration
    params:
      - { name: Mode, desc: "Selects the filter type. Available modes: LowPass (0), HighPass (1), LowShelf (2), HighShelf (3), Peak (4), ResoLow (5), StateVariableLP (6), StateVariableHP (7), MoogLP (8), OnePoleLowPass (9), OnePoleHighPass (10), StateVariableNotch (12), StateVariableBandPass (13), Allpass (14), LadderFourPoleLP (15), RingMod (17). Modes 11 (StateVariablePeak) and 16 (LadderFourPoleHP) are non-functional - selecting them leaves the previous filter type active.", range: "0 - 17", default: "6 (StateVariableLP)" }
      - { name: Frequency, desc: "The cutoff or centre frequency of the filter. At the default of 20000 Hz, all audible frequencies pass through unchanged.", range: "20 - 20000 Hz", default: "20000 Hz" }
      - { name: Q, desc: "The resonance or bandwidth of the filter. Higher values produce a sharper peak at the cutoff frequency. The effect varies by filter type.", range: "0.3 - 8.0", default: "1.0" }
      - { name: Gain, desc: "Filter gain in decibels. Only affects LowShelf, HighShelf, and Peak modes. Has no audible effect on all other filter types.", range: "-18 - 18 dB", default: "0 dB" }
  - label: Frequency Modulation
    params:
      - { name: BipolarIntensity, desc: "Scales the output of the Bipolar Freq Modulation chain. At 0 (default), bipolar modulation is disabled. Positive and negative values shift the cutoff frequency up or down in normalised frequency space before the standard Frequency Modulation chain is applied.", range: "-1.0 - 1.0", default: "0.0" }
  - label: Quality (Vestigial)
    params:
      - { name: Quality, desc: "This parameter has no effect. The coefficient update rate is fixed internally regardless of this setting.", range: "0 - 4096", default: "512" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Frequency Modulation", desc: "Scales the normalised cutoff frequency. At 1.0 (unmodulated), the full Frequency value is used. At 0.0, the frequency drops to the bottom of the range. Applied after the bipolar offset.", scope: "per-voice", constrainer: "*" }
  - { name: "Gain Modulation", desc: "Scales the filter gain for shelf and peak modes. At 1.0 (unmodulated), the full Gain value is used. Has no audible effect on filter types that do not use gain.", scope: "per-voice", constrainer: "*" }
  - { name: "Bipolar Freq Modulation", desc: "Adds a signed offset to the normalised frequency before the standard Frequency Modulation scaling. Uses offset mode (centred around 0, range -1 to +1). Only active when BipolarIntensity is non-zero.", scope: "per-voice", constrainer: "*" }
  - { name: "Q Modulation", desc: "Scales the Q value directly. At 1.0 (unmodulated), the full Q value is used.", scope: "per-voice", constrainer: "*" }
---
::

### Frequency Modulation

The two frequency modulation paths interact in a specific order: the Frequency value is normalised to a 0-1 range, the bipolar offset is added, and then the Frequency Modulation chain multiplies the result. The standard Frequency Modulation chain scales from ~20 Hz up to the Frequency knob value - it does not modulate around the set frequency. To modulate around a centre frequency, use the Bipolar Frequency chain instead. [1]($FORUM_REF.2749$)

### CPU and Polyphonic Processing

The module automatically detects whether polyphonic processing is needed based on the modulator types in its chains. Adding even a single polyphonic modulator (e.g., velocity) to any chain forces all voices to be processed independently, significantly increasing CPU cost. [2]($FORUM_REF.404$)

### Non-Functional Elements

The `Quality` parameter is vestigial - it is stored and appears in the interface but does not affect processing. Two filter modes are non-functional: **StateVariablePeak** (index 11) and **LadderFourPoleHP** (index 16). Selecting either does not change the active filter. Use Peak (index 4) or StateVariableHP (index 7) as alternatives.

**See also:** $MODULES.CurveEq$ -- Monophonic multi-band parametric EQ with visual editor - use when you need multiple filter bands on the master signal rather than per-voice filtering, $MODULES.HarmonicFilter$ -- Polyphonic filter bank tuned to the harmonic series - use for harmonic-aware spectral shaping rather than a single cutoff frequency, $SN.filters.svf$ -- scriptnode state variable filter with superior modulation stability, $SN.filters.biquad$ -- scriptnode biquad filter with six modes, $SN.filters.ladder$ -- scriptnode 24 dB/oct ladder filter, $SN.filters.moog$ -- scriptnode Moog-style transistor ladder filter
