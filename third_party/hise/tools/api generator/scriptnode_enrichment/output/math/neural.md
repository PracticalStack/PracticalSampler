---
title: Neural
description: "Runs per-sample neural network inference using an RTNeural model with optional DC-blocking high-pass filter."
factoryPath: math.neural
factory: math
polyphonic: true
tags: [math, neural, rtneural, machine-learning, inference]
cpuProfile:
  baseline: high
  polyphonic: true
  scalingFactors:
    - { parameter: "Model", impact: "dominant", note: "CPU is almost entirely determined by the neural network model size and architecture" }
seeAlso:
  - { id: "math.expr", type: alternative, reason: "Lightweight per-sample processing via a formula instead of a neural network" }
commonMistakes:
  - title: "Missing RTNeural build flag"
    wrong: "Adding math.neural to a network without enabling RTNeural support in the project build"
    right: "Enable HISE_INCLUDE_RT_NEURAL in the project settings before using this node."
    explanation: "Without the build flag, a stub implementation is compiled that performs no processing. The node will appear in the graph but will not run any inference."
  - title: "No model loaded produces silence"
    wrong: "Expecting the node to produce silence or throw an error when no model is connected"
    right: "When no model is loaded, the signal passes through unmodified."
    explanation: "The node acts as a transparent passthrough when no neural network model is connected. This is by design, allowing networks to function during development before a model is assigned."
llmRef: |
  math.neural

  Per-sample neural network inference using RTNeural. Processes all channels independently. Optional built-in high-pass filter for DC removal after inference. Requires HISE_INCLUDE_RT_NEURAL build flag.

  Signal flow:
    audio in -> [if model loaded] neural inference -> [if HPF enabled] high-pass filter -> audio out
    audio in -> [no model] passthrough -> audio out

  CPU: high, polyphonic. Dominated by model size and architecture.

  Parameters:
    None.

  Properties:
    Model: Neural network model identifier.
    HpfFreq: DC-blocking high-pass filter (Off, 1 Hz, 5 Hz, or Dynamic).

  When to use:
    Amp modelling, nonlinear effects processing, or other tasks where a trained neural network model replaces traditional DSP. Requires a pre-trained RTNeural model.

  Common mistakes:
    Must enable HISE_INCLUDE_RT_NEURAL build flag.
    No model loaded = passthrough, not silence.

  See also:
    [alternative] math.expr - lightweight per-sample formula processing
---

Runs per-sample neural network inference using an RTNeural model. Each channel is processed independently through its own network instance, and in polyphonic contexts each voice receives a separate clone of the network. This enables use cases such as amp modelling, nonlinear effects processing, or learned transfer functions that replace traditional DSP algorithms with trained models.

An optional built-in high-pass filter can be enabled to remove DC offset that neural network models sometimes introduce. When no model is loaded, the node passes the signal through unmodified - it does not produce silence or errors.

> [!Warning:Enable RTNeural in your project build] This node requires the `HISE_INCLUDE_RT_NEURAL` build flag to be enabled. Without it, a no-op stub is compiled and the node performs no processing.

## Signal Path

::signal-path
---
glossary:
  functions:
    neuralInference:
      desc: "Per-sample neural network inference on each channel"
    highPassFilter:
      desc: "Second-order high-pass filter for DC removal after inference"
---

```
// math.neural - neural network inference
// audio in -> audio out

process(input) {
    if no model loaded:
        output = input  // passthrough

    output = neuralInference(input)

    if HpfFreq != Off:
        output = highPassFilter(output)
}
```

::

## Properties

| Property | Description | Values |
|----------|-------------|--------|
| Model | The neural network model to use for inference. | Model identifier string |
| HpfFreq | DC-blocking high-pass filter applied after inference. Removes low-frequency drift that some models introduce. | Off, 1 Hz, 5 Hz, Dynamic |

Each voice maintains its own clone of the neural network, so per-voice state is fully independent. The number of network instances scales with the voice count multiplied by the channel count, which can result in significant memory usage for large models in polyphonic configurations.

### High-pass Filter

The high-pass filter options provide fixed-frequency DC blocking at 1 Hz (very gentle, removes only the slowest drift) or 5 Hz (more aggressive). The Dynamic option allows the filter frequency to be changed at runtime.

### Limitations

The maximum channel count for the built-in high-pass filter is four. Channels beyond the fourth bypass the filter but still receive neural network inference.

**See also:** $SN.math.expr$ -- lightweight per-sample formula processing
