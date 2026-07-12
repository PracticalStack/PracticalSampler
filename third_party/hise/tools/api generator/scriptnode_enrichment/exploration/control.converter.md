# control.converter - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1239`
**Base class:** `mothernode`, `pimpl::templated_mode`, `pimpl::no_mod_normalisation`, `pimpl::parameter_node_base<ParameterClass>`, `pimpl::no_processing`
**Classification:** control_source

## Signal Path

Value parameter (raw, unscaled) -> ConverterClass::getValue(input) -> modulation output (unnormalised).

The `setValue()` method at line 1276:

```
auto v = obj.getValue(input);
if (this->getParameter().isConnected())
    this->getParameter().call(v);
```

The conversion is delegated to the `ConverterClass` template argument which is selected by the Mode property.

## Gap Answers

### converter-mode-variants: What are all available Mode options?

The `conversion_logic::dynamic` class (logic_classes.h:182) defines 14 modes:

| Index | Mode | Formula | Input Domain | Output Domain |
|-------|------|---------|--------------|---------------|
| 0 | Ms2Freq | 1000 / input | Milliseconds | Hz |
| 1 | Freq2Ms | 1000 / input | Hz | Milliseconds |
| 2 | Freq2Samples | sampleRate / input | Hz | Samples |
| 3 | Ms2Samples | input * 0.001 * sampleRate | Milliseconds | Samples |
| 4 | Samples2Ms | input / sampleRate * 1000 | Samples | Milliseconds |
| 5 | Ms2BPM | 60000 / max(input, 1) | Milliseconds | BPM |
| 6 | Pitch2St | log2(input) * 12 | Pitch factor | Semitones |
| 7 | St2Pitch | 2^(input/12) | Semitones | Pitch factor |
| 8 | Pitch2Cent | log2(input) * 1200 | Pitch factor | Cents |
| 9 | Cent2Pitch | 2^(input/1200) | Cents | Pitch factor |
| 10 | Midi2Freq | MidiMessage::getMidiNoteInHertz(round(input*127)) | 0..1 normalised | Hz |
| 11 | Freq2Norm | input / 20000 | Hz | 0..1 normalised |
| 12 | Gain2dB | hmath::gain2db(input) | Linear gain | dB |
| 13 | dB2Gain | hmath::db2gain(input) | dB | Linear gain |

Note: Ms2Freq, Freq2Ms, Ms2BPM protect against division by zero by checking for zero input. Modes 2, 3, 4 require PrepareSpecs (sample rate dependent).

### converter-input-output-domains: Expected input and output domains per mode?

Documented in the table above. All conversions operate on raw (unnormalised) values. The input is unscaled (marked via `no_mod_normalisation(getStaticId(), { "Value" })`). The output is unnormalised. Users must ensure the connected source provides values in the correct domain for the selected mode.

### converter-template-compilation: Does the Mode property map to a compile-time template argument?

Yes. The `templated_mode` base (line 1253) registers `HasModeTemplateArgument` with mode namespace `"conversion_logic"`. At compile time, each mode becomes a separate C++ class (e.g., `converter<PT, conversion_logic::ms2freq>`), so the conversion logic is fully inlined with no runtime branching. At runtime in the interpreter, `conversion_logic::dynamic` is used which switches on the mode enum. The `usesPrepareSpecs()` constexpr check (line 1260) allows compile-time elimination of `prepare()` overhead for modes that do not need sample rate.

## Parameters

- **Value**: Raw input. Unscaled (receives values without range conversion). Uses `SN_ADD_SET_VALUE`.

## Properties

- **Mode**: Selects conversion formula. Default "Ms2Freq". Maps to `conversion_logic` namespace classes at compile time.

## Conditional Behaviour

The Mode property selects one of 14 conversion formulas. Three modes (Ms2Samples, Freq2Samples, Samples2Ms) depend on sample rate and require `prepare()`. The `usesPrepareSpecs()` constexpr detects this at compile time. When sample-rate-dependent modes are used, the converter stores `lastInput` and re-applies the conversion when `prepare()` is called (line 1270-1273).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The converter node is monophonic (uses `SN_NODE_ID`, not `SN_POLY_NODE_ID`). It has no `PolyData` member. The Midi2Freq mode notably takes a normalised 0..1 input (multiplied by 127 internally), which is unusual for an unscaled input node. This means connecting an unnormalised MIDI note number would require dividing by 127 first, or the user should use this mode with a normalised source.
