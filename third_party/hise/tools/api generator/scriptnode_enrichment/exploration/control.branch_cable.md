# control.branch_cable - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1721` (branch_cable class)
**Base class:** `branch_base<ParameterClass>` (which inherits `parameter_node_base<PC>`, `no_processing`), `polyphonic_base`
**Classification:** control_source

## Signal Path

The branch_cable node routes a single input Value to one of multiple output slots selected by the Index parameter. It uses `callWithRuntimeIndex(index, value)` to send the value to exactly one output. Other outputs are NOT reset when the index changes -- they retain their last received value.

Index parameter -> selects output slot (integer)
Value parameter -> routed to selected output slot via callWithRuntimeIndex()

## Gap Answers

### branch-routing-logic: Does branch_cable use callWithRuntimeIndex? What happens to other outputs?

Yes, confirmed. `setValue(double newValue)` (line 1747-1761) iterates all voices to store the value and get the current index, then calls `this->getParameter().callWithRuntimeIndex(index, newValue)` (line 1759). Only the selected output slot receives the new value. Other outputs retain whatever value they last received -- there is no reset or zeroing of unselected outputs.

### branch-index-clamping: What happens if Index exceeds NumParameters?

Bounds-checked via `isPositiveAndBelow(index, this->getParameter().getNumParameters())` (line 1757). If the index is out of range (negative or >= NumParameters), the call is silently skipped -- no output is sent. The value is still stored in per-voice state but not forwarded.

### branch-numparameters-init: How is NumParameters set?

The `initialise()` method (line 1740-1743) calls `this->getParameter().initialise(n)` which initializes the dynamic_list parameter system. The NumParameters property is stored in the ValueTree and must be configured by the user in the scriptnode UI. It is not auto-detected from connections. The dynamic_list system reads it from the ValueTree during initialization.

### branch-normalisation: Is branch_cable output normalised?

Yes, normalised. The branch_cable class does NOT inherit from `no_mod_normalisation` and does NOT override `isNormalisedModulation()`. The default from `no_processing` applies: `isNormalisedModulation() == true` (line 121 of CableNodeBaseClasses.h). Connected targets will have their range conversion applied.

## Parameters

- **Value** (P=1, but registered first in createParameters): The value to route. Range 0..1, default unset. Note: createParameters registers Value first (line 1796), then Index (line 1801). However, the `setParameter<P>` template (line 1783-1789) maps P=0 to setIndex and P=1 to setValue.
- **Index** (P=0): Output slot selector. Range 0..7, step 1.0, default 0.0. Rounded to integer via `roundToInt()`.

Note: There is a parameter ordering discrepancy. `createParameters()` adds Value first (index 0 in the data list) and Index second (index 1), but `setParameter<P>` maps P=0 to setIndex and P=1 to setValue. The DEFINE_PARAMETERDATA macro binds via `SN_FORWARD_PARAMETER_TO_MEMBER` which uses the enum order. The enum defines Index=0, Value=1 (line 1734-1738). The mismatch between createParameters order and enum order may cause the runtime interpreter to swap which knob controls which behavior.

## Conditional Behaviour

When `setIndex()` is called (line 1763-1776), it stores the new index and immediately re-sends the current value to the new output. This ensures the newly selected output receives the current value without waiting for a Value parameter change.

## Polyphonic Behaviour

Uses `PolyData<Data, NV>` where Data contains `value` and `index`. Each voice can route to a different output slot independently.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The parameter registration order in `createParameters()` (Value first, Index second) does not match the Parameters enum (Index=0, Value=1). This is flagged in issues.md. In the compiled path, template connections use the enum indices, so the compiled behavior is correct. In the interpreted path, the `DEFINE_PARAMETERDATA` macro and `SN_FORWARD_PARAMETER_TO_MEMBER` should correctly bind via the enum regardless of add order, so this should be functionally correct.
