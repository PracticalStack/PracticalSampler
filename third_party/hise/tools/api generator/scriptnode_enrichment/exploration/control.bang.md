# control.bang - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1927` (multilogic::bang) + line 2742 (type alias)
**Base class:** `multi_parameter<NV, ParameterType, multilogic::bang>` which inherits `parameter_node_base<PT>`, `no_processing`, `polyphonic_base`, `combined_parameter_base<multilogic::bang>`
**Classification:** control_source

## Signal Path

The bang node stores a Value and fires it to the output when the Bang parameter transitions above 0.5. The multilogic::bang class sets `dirty = true` only when Bang > 0.5 (rising edge). The `sendPending()` method in multi_parameter then calls `getParameter().call(d.getValue())` which returns the stored `value` field directly with no transformation.

Value parameter -> stored in `value` member
Bang parameter -> if v > 0.5, sets `dirty = true`
sendPending() -> if dirty, calls output with getValue() which returns `value` unchanged

## Gap Answers

### bang-trigger-edge: Does the Bang parameter trigger on any change, only on rising edge, or on every call?

Rising-edge only (above 0.5 threshold). In `multilogic::bang::setParameter<1>(double v)` (line 1973): `dirty = v > 0.5;`. This means:
- 0->1 transition: dirty = true (fires)
- 1->0 transition: dirty = false (does not fire)
- Repeated 1->1: dirty = true on each call (fires again if sendPending runs)

However, since `dirty` is set to `v > 0.5` (not `dirty |=`), any call with v > 0.5 sets dirty, and any call with v <= 0.5 clears dirty. The trigger fires when Bang is set to a value above 0.5.

### value-passthrough-behaviour: Does it send Value directly or apply transformation?

Direct passthrough with no transformation. `getValue()` (line 1944-1948) simply returns `value` after clearing the dirty flag. No clamping, scaling, or modification is applied. The output is unnormalised (`isNormalisedModulation() == false`).

### polyphonic-bang-voice-handling: Does the bang trigger independently per voice?

Yes, per-voice independent state. The multi_parameter template wraps the DataType in `PolyData<DataType, NV>` (line 2732). In `setParameterStatic<P>()` (line 2652-2660), the parameter is set on all voices via `for(auto& s : typed->data)`, but `sendPending()` only reads the current voice's data via `data.get()`. In polyphonic mode, sendPending() only fires when `polyHandler->getVoiceIndex() != -1` (line 2669), meaning it only sends for the actively-rendering voice.

## Parameters

- **Value** (P=0): Stored as `value`. Range 0..1, default 0.0. Receives raw values (unscaled input). No clamping applied.
- **Bang** (P=1): Trigger input. Range 0..1, step 1.0. When set above 0.5, marks dirty flag. Does not store the bang value itself.

## Polyphonic Behaviour

Uses `PolyData<multilogic::bang, NV>`. Each voice has independent `value` and `dirty` state. Parameter changes propagate to all voices (via iterator in setParameterStatic), but output only fires for the active voice during rendering.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The `no_mod_normalisation` base in multilogic::bang registers "Value" as an unscaled input parameter (line 1933). The output is also unnormalised. The bang node is conceptually a sample-and-hold trigger: it stores a value and emits it on demand.
