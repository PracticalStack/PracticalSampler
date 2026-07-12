---
title: Polyshape FX
moduleId: PolyshapeFX
type: Effect
subtype: VoiceEffect
tags: [dynamics]
builderPath: b.Effects.PolyshapeFX
screenshot: /images/v2/reference/audio-modules/polyshapefx.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: Oversampling, impact: high, note: "4x oversampling quadruples the shaping cost per voice; with high polyphony this becomes significant" }
    - { parameter: VoiceCount, impact: high, note: "All processing is per-voice with no shared state; CPU scales linearly with active voices" }
seeAlso:
  - { id: ShapeFX, type: alternative, reason: "Monophonic variant with pre-shaper filtering, autogain, dry/wet mix, bit reduction, and configurable oversampling factors" }
  - { id: Saturator, type: alternative, reason: "Simpler monophonic saturation effect without table-based modes or per-voice processing" }
commonMistakes:
  - title: "Unregistered modes silently give linear"
    wrong: "Selecting Tanh, Saturate, Square, or SquareRoot modes and expecting distortion"
    right: "Use one of the registered modes: Linear, Atan, Sin, Asinh, TanCos, Chebyshev 1-3, Curve, or Asymmetrical Curve"
    explanation: "These modes are valid in ShapeFX but are not registered in Polyshape FX. Selecting them silently gives Linear passthrough with no audible effect."
  - title: "Oversampling scales CPU per voice"
    wrong: "Enabling oversampling with high polyphony and wondering about CPU spikes"
    right: "Reserve oversampling for patches with moderate voice counts, or reduce polyphony when oversampling is on"
    explanation: "Each voice has its own oversampler instance. At 4x oversampling with 16 voices, the shaping function runs 64x more than a single non-oversampled voice."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedPolyFX
  complexity: medium
  description: "A polyphonic scriptnode network with a waveshaper node inside an oversampling wrapper can replicate the core behaviour, though the mode-dependent drive/bias ordering requires custom logic"
llmRef: |
  Polyshape FX (VoiceEffect)

  A polyphonic waveshaper that processes each voice independently. Provides 10 shaping modes including mathematical curves and two user-defined table lookups. Includes a modulatable drive stage, optional 4x oversampling, DC bias, and automatic output compensation.

  Signal flow (per voice):
    audio in -> drive * modulation -> bias -> [oversample 4x up] -> waveshaper(Mode) -> [oversample 4x down] -> drive-dependent attenuation -> [DC removal] -> audio out

  CPU: low baseline per voice (no oversampling). 4x oversampling roughly quadruples cost per voice. Scales linearly with active voice count.

  Parameters:
    Drive (0 - 60 dB, default 0 dB) - distortion drive amount, modulatable at audio rate
    Mode (0 - 33 discrete, default 1: Linear) - shaping function selector (10 active modes, unregistered modes give Linear passthrough)
    Oversampling (Off/On, default Off) - enables 4x oversampling for reduced aliasing
    Bias (0 - 100%, default 0%) - DC offset before shaping for asymmetric distortion

  Modulation chains:
    Drive Modulation (audio-rate, per-voice) - scales the drive amount from unity (mod=0) to full drive (mod=1)

  Available modes: Linear (1), Atan (2), Sin (4), Asinh (5), TanCos (9), Chebyshev 1 (10), Chebyshev 2 (11), Chebyshev 3 (12), Curve (32), Asymmetrical Curve (33). All other indices give Linear passthrough.

  When to use:
    Per-voice distortion, saturation, or waveshaping where each voice needs independent processing. Use Curve/Asymmetrical Curve for custom transfer functions. Use ShapeFX instead for monophonic bus processing with filtering and dry/wet mix.

  Common mistakes:
    Unregistered modes (Tanh, Saturate, Square, SquareRoot) silently give Linear. Oversampling cost scales with voice count.

  Custom equivalent:
    scriptnode HardcodedPolyFX with waveshaper and oversampling nodes.

  See also:
    alternative ShapeFX - monophonic variant with filtering, autogain, mix, and more oversampling options
    alternative Saturator - simpler monophonic saturation
---

::category-tags
---
tags:
  - { name: dynamics, desc: "Effects that shape the amplitude or add distortion and saturation" }
---
::

![Polyshape FX screenshot](/images/v2/reference/audio-modules/polyshapefx.png)

Polyshape FX is a polyphonic waveshaper that processes each voice independently through a nonlinear shaping function. It provides 10 active shaping modes - 8 mathematical curves and 2 user-defined table lookups - with a modulatable drive stage and optional 4x oversampling for reduced aliasing. Unlike its monophonic counterpart Shape FX, it has no pre-shaper filtering, no dry/wet mix, no bit reduction, and no autogain. The output is always fully wet with automatic drive-dependent level compensation.

The drive parameter accepts audio-rate modulation, allowing per-voice dynamic control of the distortion intensity. The two table-based modes (Curve and Asymmetrical Curve) use separate lookup tables edited in the module's panel: Curve applies symmetric shaping (same curve for positive and negative signal), while Asymmetrical Curve allows different shaping for each polarity.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Drive:
      desc: "Distortion drive amount in decibels"
      range: "0 - 60 dB"
      default: "0 dB"
    Mode:
      desc: "Selects the waveshaping function"
      range: "0 - 33 (discrete)"
      default: "1 (Linear)"
    Oversampling:
      desc: "Enables 4x oversampling for reduced aliasing"
      range: "Off / On"
      default: "Off"
    Bias:
      desc: "DC offset before shaping for asymmetric distortion"
      range: "0 - 100%"
      default: "0%"
  functions:
    shapeSample:
      desc: "Applies the selected nonlinear shaping function to each sample"
    removeDC:
      desc: "Removes DC offset with a 20 Hz high-pass filter (per voice)"
    attenuate:
      desc: "Drive-dependent output compensation to reduce level boost from high drive"
  modulations:
    DriveModulation:
      desc: "Audio-rate scaling of the drive amount"
      scope: "per-voice"
---

```
// Polyshape FX - per-voice waveshaper
// stereo in (per voice) -> stereo out (per voice)

process(left, right) {
    // Drive buffer: modulation scales between unity and full drive
    driveGain = 1 + DriveModulation * (dBToLinear(Drive) - 1)

    // Drive and bias application (mode-dependent order)
    if Mode == Sin or Mode == TanCos:
        signal *= driveGain
        signal += Bias
    else:
        signal = (signal + Bias) * (1 + driveGain)

    // Waveshaping (optionally oversampled)
    if Oversampling:
        signal = upsample4x(signal)

    signal = shapeSample(signal, Mode)

    if Oversampling:
        signal = downsample4x(signal)

    // Output compensation
    signal = attenuate(signal, driveGain)

    // DC removal (conditional)
    if Bias != 0 or Mode == AsymmetricalCurve:
        signal = removeDC(signal)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Shaping
    params:
      - { name: Drive, desc: "Distortion drive amount in decibels. Controls how hard the signal is driven into the shaping function. Modulatable at audio rate via the Drive Modulation chain. At 0 dB the signal passes through the shaper at unity level.", range: "0 - 60 dB", default: "0 dB" }
      - name: Mode
        desc: "Selects the waveshaping function. Available modes: Linear (1), Atan (2), Sin (4), Asinh (5), TanCos (9), Chebyshev 1 (10), Chebyshev 2 (11), Chebyshev 3 (12), Curve (32), Asymmetrical Curve (33). All other indices give Linear passthrough."
        range: "0 - 33 (discrete)"
        default: "1 (Linear)"
        hints:
          - type: warning
            text: "Modes 3 (Tanh), 6 (Saturate), 7 (Square), and 8 (SquareRoot) are visible in the range but not registered in this module. Selecting them gives silent Linear passthrough. These modes are functional in $MODULES.ShapeFX$ only."
          - type: info
            text: "Curve (32) uses Table 0, mapping absolute input amplitude to output (symmetric around zero). Asymmetrical Curve (33) uses Table 1, mapping the full -1 to +1 range for different positive/negative shaping."
      - { name: Oversampling, desc: "Enables 4x oversampling to reduce aliasing artefacts from the shaping function. Each voice has its own oversampler, so CPU cost scales with voice count.", range: "Off / On", default: "Off" }
  - label: Bias
    params:
      - { name: Bias, desc: "DC offset added to the signal before the shaping function. Creates asymmetric distortion by shifting the signal into a non-centred region of the transfer curve. DC offset is automatically removed after shaping when bias is active.", range: "0 - 100%", default: "0%" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: Drive Modulation, desc: "Scales the drive amount from unity (modulation = 0) to the full Drive value (modulation = 1). Operates at audio rate for per-sample drive control within each voice.", scope: "per-voice", constrainer: "All" }
---
::

### Drive and Bias Interaction

The drive and bias application order differs by mode. Sin and TanCos apply drive first then add bias, while all other modes add bias first then multiply by drive. This means the same Drive and Bias settings produce different tonal results depending on the selected mode.

The post-shaping output compensation uses a simple drive-dependent divisor that provides mild level reduction at high drive values, rather than computing a static inverse from the transfer function like $MODULES.ShapeFX$.

**See also:** $MODULES.ShapeFX$ -- Monophonic variant with pre-shaper filtering, autogain, dry/wet mix, bit reduction, and configurable oversampling factors (1x-16x), $MODULES.Saturator$ -- Simpler monophonic saturation effect without table-based modes or per-voice processing
