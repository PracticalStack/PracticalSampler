---
title: Repitch
description: "A serial container that resamples audio before and after child processing to change the effective pitch."
factoryPath: container.repitch
factory: container
polyphonic: false
tags: [container, serial, resampling]
cpuProfile:
  baseline: medium
  polyphonic: false
  scalingFactors:
    - { parameter: "Interpolation", impact: "quality-dependent", note: "Cubic uses 4-point interpolation per sample; None uses nearest-neighbour" }
seeAlso: []
commonMistakes:
  - title: "Using more than two audio channels"
    wrong: "Placing a repitch container in a context with more than 2 channels and expecting resampling to work"
    right: "Use repitch only in mono or stereo contexts. For multichannel resampling, nest multiple repitch containers inside a container.multi."
    explanation: "Repitch only processes 1 or 2 channels. With more channels, the audio passes through unmodified with no warning."
llmRef: |
  container.repitch

  A serial container that resamples audio before and after child processing. Children see a different effective sample rate depending on the RepitchFactor.

  Signal flow:
    input -> downsample(ratio) -> children.process -> upsample(ratio) -> output

  CPU: medium, monophonic
    Resampling cost depends on interpolation quality.

  Parameters:
    RepitchFactor: 0.5 - 2.0 (default 1.0, logarithmic skew)
      Factor > 1.0: children see lower effective pitch (fewer internal samples).
      Factor < 1.0: children see higher effective pitch (more internal samples).
    Interpolation: Cubic / Linear / None (default Cubic)
      Resampling quality. Cubic is highest quality; None introduces aliasing artifacts.

  When to use:
    Change the frequency response of static nodes (convolution, neural networks), alter time-based effects, or introduce resampling artifacts. Stack multiple repitch containers for a range beyond one octave.

  Key details:
    Primary use case: shift frequency response of SR-dependent effects (filters, convolution, neural nets) without changing coefficients.
    SR-independent nodes (gain, waveshapers) are unaffected by the repitch factor.
    Frequency response shift may appear inverted when only effects (no generator) are inside.

  Common mistakes:
    Only works with 1 or 2 channels. Multichannel audio passes through unmodified.

  See also:
    (none)
---

The repitch container resamples the audio signal before and after child processing, effectively changing the pitch that children perceive. This is useful for altering the frequency response of nodes that cannot be tuned directly (such as convolution reverbs or neural networks), changing the time response of delay-based effects, or intentionally introducing resampling artifacts.

The `RepitchFactor` parameter controls the resampling ratio within a one-octave range (0.5 to 2.0). A factor above 1.0 produces fewer internal samples, so children hear a lower effective pitch. A factor below 1.0 produces more internal samples, raising the effective pitch. At 1.0, no resampling occurs. For a wider range, stack multiple repitch containers inside each other.

Three interpolation modes are available:

- **Cubic** (default): highest quality, 4-point interpolation
- **Linear**: mid quality, 2-point interpolation
- **None**: nearest-neighbour, lowest quality - useful for intentional digital distortion artifacts

## Signal Path

::signal-path
---
glossary:
  parameters:
    RepitchFactor:
      desc: "Resampling ratio that controls the effective pitch change"
      range: "0.5 - 2.0"
      default: "1.0"
    Interpolation:
      desc: "Interpolation quality for the resampling operation"
      range: "Cubic / Linear / None"
      default: "Cubic"
  functions:
    downsample:
      desc: "Resamples input to a different number of internal samples"
    upsample:
      desc: "Resamples the processed result back to the original sample count"
---

```
// container.repitch - pitch-shifting via resampling
// audio in -> audio out

dispatch(input) {
    internalSamples = round(numSamples / RepitchFactor)
    downsample(input, internalSamples, Interpolation)
    children.process(internalBuffer)    // serial, at effective SR
    upsample(internalBuffer, numSamples, Interpolation)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - name: RepitchFactor
        desc: "Resampling ratio. Above 1.0 lowers effective pitch; below 1.0 raises it. Logarithmic skew centred at 1.0."
        range: "0.5 - 2.0"
        default: "1.0"
        hints:
          - type: tip
            text: "The primary use case is shifting the frequency response of sample-rate-dependent effects (filters, convolution reverbs, neural network models) without modifying their coefficients. A factor of 0.5 shifts the response one octave down; 2.0 shifts it one octave up."
      - { name: Interpolation, desc: "Resampling quality. Cubic is highest quality; None produces aliasing artifacts.", range: "Cubic / Linear / None", default: "Cubic" }
---
::

### Limitations

- Repitch only processes mono or stereo signals. Audio with more than 2 channels passes through unmodified.
- Repitch containers cannot be nested inside frame-based containers.
- Nodes that do not depend on sample rate -- such as gain stages, waveshapers, or static lookup tables -- produce identical output regardless of the repitch factor. Only algorithms whose behaviour is tied to sample rate (filters, convolution reverbs, neural network models) are affected.

The one-octave range can be extended by stacking repitch containers. When the repitch container holds only effects (no sound generator), the frequency response shift may appear inverted relative to the factor. Placing a sound generator inside the container normalises the shift direction.

