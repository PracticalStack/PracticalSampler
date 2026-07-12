# math.neural - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:808-1111`
**Base class:** `runtime_target::indexable_target<...>`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Real-time neural network inference per sample. For each sample across ALL channels:
1. The neural network's `process(networkIndex, &input, &output)` is called in-place.
2. Each voice has a separate network clone offset (`voiceIndexOffsets`).
3. Each channel within a voice has its own network instance (offset + channelIndex).
4. If the optional high-pass filter is enabled, a 2nd-order HPF is applied per-channel after inference.

No model loaded = full passthrough (the `process()` method checks `currentNetwork != nullptr` and skips processing if null). No error is thrown at runtime for a missing model in the Dynamic HpfFrequency mode.

When `FixHpfType != Dynamic` and no model is connected, `prepare()` throws `Error::NoNeuralNetwork` (unless previously initialised successfully).

## Gap Answers

### model-format: What model format does math.neural expect?

The node connects to a `NeuralNetwork` object via the `runtime_target` system. The Model property is a string that identifies which NeuralNetwork runtime target to connect to. The actual model format (RTNeural JSON, etc.) is handled by the `NeuralNetwork` class, not by the neural node itself. The node calls `originalNetwork->clone(numClones)` to create per-voice copies.

### hpf-freq-values: What values does the HpfFreq property accept?

HpfFrequency is an enum with four values:
- **Dynamic** (-1): Allows runtime changes to HPF frequency via `setHpfFrequency()`.
- **Off** (0): No high-pass filter.
- **Hz1** (1): 1 Hz high-pass filter (removes very slow DC drift).
- **Hz5** (5): 5 Hz high-pass filter (removes DC offset more aggressively).

The filter is a 2nd-order Butterworth-style HPF computed from bilinear transform coefficients (lines 1022-1039).

### is-fix-runtime-target: What does IsFixRuntimeTarget mean?

The node uses the `runtime_target::indexable_target` system to connect to a `NeuralNetwork` instance. `IsFixRuntimeTarget` means it connects to a fixed (compile-time determined) runtime target slot for the neural network model, rather than a dynamically resolved one.

### channel-processing: How many channels does math.neural process?

ALL channels. The `process()` method iterates `for(auto& ch: data)` (line 957), processing every channel. Each channel gets its own network instance: the voice offset plus the channel index. The SN_DESCRIPTION says "first channel" but the code processes all channels. This is a description inaccuracy.

### no-model-behaviour: What happens when no model is loaded?

Passthrough. The `process()` and `processFrame()` methods check `currentNetwork != nullptr` and also verify that `getNumExpectedNetworks() == currentNetwork->getNumNetworks()`. If either check fails, processing is skipped entirely and the signal passes through unchanged. No error, no silence.

Exception: when `FixHpfType != Dynamic` and the node was never successfully initialised, `prepare()` throws `Error::NoNeuralNetwork`.

### empty-description: What is an accurate description?

"Runs per-sample neural network inference on all channels using an RTNeural model with optional DC-blocking high-pass filter."

### rt-neural-dependency: What RTNeural details are available?

The node requires `HISE_INCLUDE_RT_NEURAL` at compile time. When disabled, a stub implementation is compiled that does nothing (lines 1090-1111). The network architectures supported depend on the `NeuralNetwork` class (not defined in MathNodes.h). The node calls `process(index, &input, &output)` per sample, suggesting single-sample inference.

### unusual-base-json-structure: Is the base JSON structure anomalous?

The node genuinely has no DSP parameters (`createParameters()` is empty, line 850-853). It has two properties (Model, HpfFreq) but zero parameters. This is correct -- the node's behaviour is configured entirely through properties and the runtime target connection.

## Polyphonic Behaviour

The node is polyphonic with `NV` voices. Key per-voice state:
- `PolyData<int, NV> voiceIndexOffsets`: Maps each voice to its starting network index. Each voice's offset is `voiceIndex * numChannels`.
- `PolyData<FilterStateArray, NV> filterState`: Per-voice HPF filter state (4 floats per channel, max 4 channels).
- Network cloning: `originalNetwork->clone(NV * numChannels)` creates one network instance per voice per channel.

On `reset()`, each voice's network instances are reset and warmed up with `HISE_NEURAL_NETWORK_WARMUP_TIME` samples.

## Conditional Behaviour

Two conditions affect processing:

1. **HpfFrequency**: When `shouldUseHpf()` returns true (hpfFrequency > Off), a 2nd-order HPF is applied per sample per channel after neural inference. When Off, the HPF is bypassed entirely. When `FixHpfType == Off` (compile-time), `shouldUseHpf()` is constexpr false.

2. **Model connection**: No model = passthrough. The network count must match `(NV or 1) * numChannels` for processing to occur.

## CPU Assessment

baseline: high
polyphonic: true
scalingFactors: [{"parameter": "Model", "impact": "dominant", "note": "CPU is almost entirely determined by the neural network model size and architecture"}]

## Notes

The node registers `polyphonic_base(getStaticId(), false)` -- the second argument `false` means it does NOT add `IsProcessingHiseEvent`. This matches `SN_EMPTY_HANDLE_EVENT` in the class. The max HPF channel count is hardcoded to 4 (`maxFilterChannels = 4`); channels beyond 4 bypass the HPF (line 1059).
