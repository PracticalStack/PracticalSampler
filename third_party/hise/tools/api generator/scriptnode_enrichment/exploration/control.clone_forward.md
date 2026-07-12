# control.clone_forward - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1606`
**Base class:** `pimpl::no_processing`, `pimpl::parameter_node_base<ParameterType>`, `wrap::clone_manager::Listener`
**Classification:** control_source

## Signal Path

Value parameter -> sendValue() -> callEachClone(i, lastValue, false) for each clone [0, numClones). Every clone receives the identical unscaled value. No distribution logic, no transformation.

## Gap Answers

### unscaled-forwarding-detail: Confirm unscaled parameter forwarding

Confirmed from C++ source. In the constructor: `this->getParameter().isNormalised = isNormalisedModulation()` where `isNormalisedModulation()` returns false (line 1616). Additionally, `UseUnnormalisedModulation` is registered via `CustomNodeProperties::setPropertyForObject` (line 1626), and "Value" is marked as an unscaled input parameter via `addUnscaledParameter` (line 1627). This means:
1. The Value input receives raw values without range scaling from the source.
2. The output to each clone is NOT range-converted through the target parameter's range.
The target parameter receives the raw value as-is. If the user wants a specific range, they must ensure the incoming value is already in the correct range.

### createparameters-copypaste: Confirm copy-paste in createParameters

Confirmed. Lines 1685 and 1691 use `DEFINE_PARAMETERDATA(clone_cable, NumClones)` and `DEFINE_PARAMETERDATA(clone_cable, Value)` instead of `clone_forward`. This is functionally harmless because the DEFINE_PARAMETERDATA macro uses the class name only for generating the parameter callback binding (via `registerCallback`), and the parameter names ("NumClones", "Value") are string literals embedded in the range data. The callback binding uses the template parameter index, not the class name, so the parameters bind correctly to clone_forward's setParameter<P> method.

## Parameters

- **NumClones** (P=0): Integer 1-16. Number of active clones. Auto-synced from parent clone container via clone_manager::Listener.
- **Value** (P=1): 0.0-1.0 range in definition, but unscaled -- receives and forwards raw values without range conversion.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: [{ "parameter": "NumClones", "impact": "linear", "note": "One callEachClone per active clone" }]

## Notes

clone_forward explicitly sets `SN_EMPTY_HANDLE_EVENT` and `isProcessingHiseEvent() = false`, unlike clone_cable. The `shouldUpdateClones()` method always returns true, so auto-sync from the parent container always works. The `numClones` member variable is not initialized in the declaration (line 1699), which is a minor code quality issue but not user-visible since setNumClones is called before any values are sent.
