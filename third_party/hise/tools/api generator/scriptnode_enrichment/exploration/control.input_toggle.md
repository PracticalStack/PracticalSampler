# control.input_toggle - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:267` (input_toggle class)
**Base class:** `input_toggle_base`, `parameter_node_base<ParameterClass>`, `no_mod_normalisation`, `polyphonic_base`, `no_processing`
**Classification:** control_source

## Signal Path

The input_toggle node selects between two stored values (Value1, Value2) based on the Input parameter. When Input < 0.5, Value1 is forwarded. When Input >= 0.5, Value2 is forwarded. The selected value is sent directly to the output with no transformation.

Input parameter -> threshold at 0.5 -> selects Value1 or Value2
Selected value -> output (unnormalised, no transformation)

## Gap Answers

### toggle-selection-logic: How does Input map to which value is selected?

Threshold-based at 0.5. In `setInput(double input)` (line 302-312): `d.useValue1 = input < 0.5`. So:
- Input < 0.5: Value1 is forwarded
- Input >= 0.5: Value2 is forwarded

The Input parameter has step=1.0 and range 0..1, so in practice it receives 0 or 1. But the threshold logic works for any continuous value.

### toggle-text-converter: What display labels does the Input parameter produce?

The text converter is set via `p.setParameterValueNames({ "Input 1", "Input 2"})` (line 352). So Input=0 displays "Input 1" and Input=1 displays "Input 2".

### toggle-output-on-input-change: Does the node immediately send the selected value on any change?

Yes, all three parameter setters immediately send the output if appropriate:
- `setInput()` (line 302): sends the newly selected value immediately
- `setValue1()` (line 314): sends Value1 only if Value1 is currently active (`d.useValue1 == true`)
- `setValue2()` (line 328): sends Value2 only if Value2 is currently active (`!d.useValue1`)

This means changing the inactive value does NOT trigger an output. Only the active value's changes propagate.

## Parameters

- **Input** (P=0): Selector. Range 0..1, step 1.0, default 0.0. Display names: "Input 1", "Input 2". Threshold at 0.5.
- **Value1** (P=1): First value. Range 0..1, default 0.0. Unscaled input (registered via `no_mod_normalisation(getStaticId(), {"Value1", "Value2"})`).
- **Value2** (P=2): Second value. Range 0..1, default 0.0. Unscaled input.

## Polyphonic Behaviour

Uses `PolyData<Data, NV>` where Data contains `useValue1`, `v1`, `v2`. Each voice has independent selection state and values. All parameter setters iterate all voices via `for(auto& d: data)`.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The node is a multiplexer: it selects one of two input signals to forward. Unlike blend (which interpolates), input_toggle does a hard switch. The output is unnormalised (`no_mod_normalisation` is inherited, Value1 and Value2 are registered as unscaled inputs).
