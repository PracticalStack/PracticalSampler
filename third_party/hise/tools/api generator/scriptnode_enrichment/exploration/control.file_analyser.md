# control.file_analyser - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:204`
**Base class:** `pimpl::no_processing`, `pimpl::parameter_node_base`, `pimpl::templated_mode`, `pimpl::no_mod_normalisation`
**Classification:** control_source

## Signal Path

Audio file loaded -> setExternalData() fires -> AnalyserType::getValue(d) extracts a property -> output parameter called with extracted value.

The file_analyser node has no parameters. It is purely event-driven: when an audio file is assigned (via setExternalData), it reads the block data and calls `analyser.getValue(d)` to extract a numeric property. If the result is non-zero, it forwards it to the output parameter.

## Gap Answers

### mode-variants: What Mode values are available?

From the `file_analysers` namespace in `logic_classes.h`, three modes are available:

1. **pitch**: Calls `PitchDetection::detectPitch(b.begin(), b.size(), d.sampleRate)`. Returns detected frequency in Hz.
2. **milliseconds**: Returns `1000.0 * (double)d.numSamples / d.sampleRate`. Duration of the file in milliseconds.
3. **peak**: Returns `b.getMagnitude(0, d.numSamples)`. Peak magnitude as a linear amplitude value (0.0 to 1.0+).

The mode namespace is `"file_analysers"` (set in templated_mode constructor).

### trigger-mechanism: When does the analysis trigger?

Analysis triggers exclusively in `setExternalData()`. This fires when an audio file is loaded or changed. It runs synchronously within the setExternalData callback. There is no periodic re-analysis or process-time analysis. If the block size is 0 (empty file), nothing is sent.

### output-range: What range do the output values have for each mode?

- **pitch**: Frequency in Hz (e.g., 440.0 for A4). Range depends on the audio content.
- **milliseconds**: Duration in ms. Range depends on file length and sample rate.
- **peak**: Linear amplitude. Typically 0.0 to 1.0 but can exceed 1.0 for clipped audio.

All outputs are unnormalised (raw values via `no_mod_normalisation`).

## Parameters

No parameters. The node is controlled entirely by the Mode property and audio file assignment.

## CPU Assessment

baseline: negligible (only runs on file load, not during audio processing)
polyphonic: false
scalingFactors: []

## Notes

The `no_mod_normalisation` constructor passes an empty list `{}` for unscaled input parameters since there are no input parameters. The node registers with mode namespace `"file_analysers"`. The `initialise()` method forwards to the AnalyserType if it has an initialise method (checked via prototypes::check).
