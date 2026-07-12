---
title: Shape FX
moduleId: ShapeFX
type: Effect
subtype: MasterEffect
tags: [dynamics]
builderPath: b.Effects.ShapeFX
screenshot: /images/v2/reference/audio-modules/shapefx.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: Oversampling, impact: high, note: "Each doubling of the oversampling factor roughly doubles the CPU cost of shaping and bitcrushing" }
seeAlso:
  - { id: PolyshapeFX, type: alternative, reason: "Polyphonic variant with per-voice processing and a drive modulation chain" }
commonMistakes:
  - title: "Autogain compensates for gain changes"
    wrong: "Increasing Gain and wondering why the output level stays the same"
    right: "Disable Autogain to hear the full effect of the Gain parameter on output level"
    explanation: "Autogain compensates for the increased output caused by higher gain, keeping the perceived level roughly constant. Disable it when you want gain to affect the output level directly."
  - title: "BypassFilters also bypasses DC removal"
    wrong: "Setting BypassFilters to On and using bias, then getting DC offset in the output"
    right: "Keep BypassFilters off when using non-zero bias values"
    explanation: "The DC removal filter is tied to BypassFilters. When filters are bypassed, the DC offset introduced by BiasLeft/BiasRight persists in the output, which can cause headroom issues downstream."
  - title: "High oversampling is CPU expensive"
    wrong: "Using high Oversampling values on multiple instances"
    right: "Use 2x or 4x oversampling, or apply higher factors to a single critical instance"
    explanation: "16x oversampling processes the shaping function at 16 times the sample rate, significantly increasing CPU usage. The quality improvement diminishes above 4x for most shaping modes."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: medium
  description: "A scriptnode network with a waveshaper node, oversampling wrapper, and pre/post filtering can replicate this module's behaviour"
llmRef: |
  Shape FX (MasterEffect)

  A monophonic waveshaper with selectable shaping modes, pre-shaper filtering, DC bias, bit reduction, oversampling, autogain, and dry/wet mix. Provides 14 shaping functions including mathematical curves and user-defined table lookups.

  Signal flow:
    audio in -> dry copy -> [HP + LP filters] -> gain -> bias -> [soft limiter] -> [oversample up] -> waveshaper -> [bitcrusher] -> [oversample down] -> [DC removal] -> [autogain] -> mix with dry -> audio out

  CPU: low baseline (1x oversampling). Oversampling scales cost significantly (high at 16x). Monophonic.

  Parameters:
    Pre-Shaper Filtering:
      HighPass (20 - 8000 Hz, default 20 Hz) - high-pass cutoff before shaping
      LowPass (200 - 20000 Hz, default 20000 Hz) - low-pass cutoff before shaping
      BypassFilters (Off/On, default Off) - bypasses HP, LP, and DC removal together
    Input Stage:
      Gain (0 - 60 dB, default 0 dB) - input gain before shaping
      BiasLeft (-1.0 - 1.0, default 0.0) - DC offset on left channel
      BiasRight (-1.0 - 1.0, default 0.0) - DC offset on right channel
      LimitInput (Off/On, default On) - soft limiter before shaping
    Shaping:
      Mode (0 - 33 discrete, default 1: Linear) - shaping function selector
      Drive (0 - 100%, default 0%) - vestigial, has no effect
      Oversampling (1x/2x/4x/8x/16x, default 1x) - anti-aliasing quality
      Reduce (0 - 14, default 0) - bit reduction depth
    Output:
      Autogain (Off/On, default On) - automatic level compensation
      Mix (0 - 100%, default 100%) - dry/wet balance

  When to use:
    Distortion, saturation, waveshaping, or lo-fi effects on a stereo bus. Use the table-based modes (Curve, Asymmetrical Curve) for custom transfer functions.

  Common mistakes:
    Autogain masks gain changes. BypassFilters also bypasses DC removal. High oversampling is CPU-expensive.

  Custom equivalent:
    scriptnode HardcodedFX with waveshaper, oversampling, and filtering nodes.

  See also:
    alternative PolyshapeFX - polyphonic variant with drive modulation
---

::category-tags
---
tags:
  - { name: dynamics, desc: "Effects that shape the amplitude or add distortion and saturation" }
---
::

![Shape FX screenshot](/images/v2/reference/audio-modules/shapefx.png)

The Shape FX is a monophonic waveshaper with 14 built-in shaping functions and two user-defined table modes. It processes stereo audio through an optional pre-shaper filter stage, applies gain and DC bias, runs the signal through the selected shaping algorithm (optionally oversampled for reduced aliasing), and mixes the result with the dry signal.

The two table-based modes (Curve and Asymmetrical Curve) allow custom transfer functions drawn in the module's editor. Bit reduction can be layered on top of any shaping mode for lo-fi effects. Autogain keeps the output level consistent as gain and mode settings change.

## Signal Path

::signal-path
---
glossary:
  parameters:
    HighPass:
      desc: "High-pass cutoff frequency applied before shaping"
      range: "20 - 8000 Hz"
      default: "20 Hz"
    LowPass:
      desc: "Low-pass cutoff frequency applied before shaping"
      range: "200 - 20000 Hz"
      default: "20000 Hz"
    BypassFilters:
      desc: "Bypasses pre-shaper HP/LP and post-shaper DC removal together"
      range: "Off / On"
      default: "Off"
    Gain:
      desc: "Input gain in decibels applied before shaping"
      range: "0 - 60 dB"
      default: "0 dB"
    BiasLeft:
      desc: "DC offset added to the left channel before shaping"
      range: "-1.0 - 1.0"
      default: "0.0"
    BiasRight:
      desc: "DC offset added to the right channel before shaping"
      range: "-1.0 - 1.0"
      default: "0.0"
    LimitInput:
      desc: "Enables a soft limiter before the shaping stage"
      range: "Off / On"
      default: "On"
    Mode:
      desc: "Selects the waveshaping function"
      range: "0 - 33 (discrete)"
      default: "1 (Linear)"
    Oversampling:
      desc: "Oversampling factor for anti-aliasing"
      range: "1x / 2x / 4x / 8x / 16x"
      default: "1x"
    Reduce:
      desc: "Bit reduction depth for lo-fi distortion"
      range: "0 - 14"
      default: "0"
    Autogain:
      desc: "Automatic output level compensation based on the shaping curve"
      range: "Off / On"
      default: "On"
    Mix:
      desc: "Dry/wet balance"
      range: "0 - 100%"
      default: "100%"
  functions:
    shapeSample:
      desc: "Applies the selected nonlinear shaping function to each sample"
    bitcrush:
      desc: "Quantises the signal to a reduced bit depth"
    removeDC:
      desc: "Removes DC offset introduced by bias and shaping (30 Hz high-pass)"
    computeAutogain:
      desc: "Calculates a static gain compensation value from the shaping curve"
---

```
// Shape FX - monophonic waveshaper
// stereo in -> stereo out

// Dry copy (before any processing)
dry = input * (1 - Mix)

// Pre-shaper filtering (wet path)
if !BypassFilters:
    wet = highPass(input, HighPass)
    wet = lowPass(wet, LowPass)

// Input stage
wet *= dBToLinear(Gain)
wet.left  += BiasLeft
wet.right += BiasRight

if LimitInput:
    wet = softLimit(wet)

// Shaping (optionally oversampled)
if Oversampling > 1x:
    wet = upsample(wet, Oversampling)

wet = shapeSample(wet, Mode)

if Reduce > 0:
    wet = bitcrush(wet, Reduce)

if Oversampling > 1x:
    wet = downsample(wet, Oversampling)

// Post-shaper
if !BypassFilters:
    wet = removeDC(wet)

if Autogain:
    wet *= computeAutogain(Mode, Gain)

// Output
output = wet * Mix + dry
```

::

## Parameters

::parameter-table
---
groups:
  - label: Pre-Shaper Filtering
    params:
      - { name: HighPass, desc: "High-pass cutoff frequency applied to the wet signal before shaping. Has no effect when BypassFilters is on.", range: "20 - 8000 Hz", default: "20 Hz" }
      - { name: LowPass, desc: "Low-pass cutoff frequency applied to the wet signal before shaping. Has no effect when BypassFilters is on.", range: "200 - 20000 Hz", default: "20000 Hz" }
      - { name: BypassFilters, desc: "Bypasses the pre-shaper high-pass and low-pass filters, and also the post-shaper DC removal filter. Enable this for a raw, unfiltered shaping path.", range: "Off / On", default: "Off" }
  - label: Input Stage
    params:
      - { name: Gain, desc: "Input gain in decibels applied before the shaping function. Higher values drive the signal harder into the shaper. When Autogain is on, output level is compensated automatically.", range: "0 - 60 dB", default: "0 dB" }
      - { name: BiasLeft, desc: "DC offset added to the left channel before shaping. Creates asymmetric distortion. DC is removed post-shaping unless BypassFilters is on.", range: "-1.0 - 1.0", default: "0.0" }
      - { name: BiasRight, desc: "DC offset added to the right channel before shaping. Independent from BiasLeft for stereo asymmetry effects.", range: "-1.0 - 1.0", default: "0.0" }
      - { name: LimitInput, desc: "Enables a soft limiter before the shaping stage to prevent hard clipping of the gained and biased signal.", range: "Off / On", default: "On" }
  - label: Shaping
    params:
      - { name: Mode, desc: "Selects the waveshaping function. Modes 1-12 are mathematical curves (Linear, Atan, Tanh, Sin, Asinh, Saturate, Square, SquareRoot, TanCos, Chebyshev 1/2/3). Mode 32 (Curve) and 33 (Asymmetrical Curve) use the editable lookup table.", range: "0 - 33 (discrete)", default: "1 (Linear)" }
      - { name: Drive, desc: "This parameter has no effect.", range: "0 - 100%", default: "0%" }
      - { name: Oversampling, desc: "Oversampling factor for reduced aliasing during waveshaping. Higher values improve quality but increase CPU usage proportionally.", range: "1x / 2x / 4x / 8x / 16x", default: "1x" }
      - { name: Reduce, desc: "Bit reduction depth. When greater than zero, quantises the shaped signal to fewer bits for lo-fi distortion. 0 means no bit reduction.", range: "0 - 14", default: "0" }
  - label: Output
    params:
      - { name: Autogain, desc: "Automatically compensates the output level based on the current shaping curve and gain setting. The compensation is recalculated when Mode or Gain changes.", range: "Off / On", default: "On" }
      - { name: Mix, desc: "Dry/wet balance. The dry signal is the unprocessed original input (before filtering and bias). 0% is fully dry, 100% is fully wet.", range: "0 - 100%", default: "100%" }
---
::

### Oversampling

When oversampling is enabled (2x or 4x), the module introduces latency proportional to the oversampling factor. The dry signal path is automatically delayed to stay aligned with the wet signal for correct mix behaviour. The overall plugin latency reported to the host increases accordingly.

### Autogain

The autogain compensation is calculated statically by probing 128 input values through the current shaper whenever Mode or Gain changes. It is not a dynamic loudness measurement. Switching modes mid-playback triggers a recalculation with a 40 ms smoothing ramp to avoid clicks.

### Custom Curves

Modes 32 (Curve) and 33 (Asymmetrical Curve) use the lookup table edited in the module's panel. When scripting, use attribute value 32 for Curve and 33 for Asymmetrical Curve - the values are not sequential from the mathematical modes (1-12). [1]($FORUM_REF.1763$) User-drawn curves can be serialised with `exportAsBase64()` and restored later for preset saving. [2]($FORUM_REF.5099$)

The DC removal filter (30 Hz high-pass) runs after the shaper but is tied to the BypassFilters toggle. If you need bias-driven asymmetric distortion with BypassFilters enabled, DC offset will remain in the output.

**See also:** $MODULES.PolyshapeFX$ -- Polyphonic variant with per-voice processing and a drive modulation chain
