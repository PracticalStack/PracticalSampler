GlobalCable::connectToModuleParameter(String processorId, NotUndefined parameterIndexOrId, NotUndefined targetObject) -> undefined

Thread safety: UNSAFE -- performs processor lookups, allocates target objects, and modifies the cable target list
Connects the cable to a specific parameter of a HISE module. The cable's normalised 0..1 value is converted through the target range defined in `targetObject` before being applied. The parameter can be specified by index (integer) or name (string). Pass parameterIndexOrId as -1 with a valid processorId to remove connections for that processor, or pass empty processorId with -1 to remove all module parameter connections.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.connectToModuleParameter("SimpleGain", "Gain", {
    "MinValue": -100.0,
    "MaxValue": 0.0,
    "SkewFactor": 5.0,
    "SmoothingTime": 50.0
});
```
Dispatch/mechanics: Removes existing `ProcessorParameterTarget` entries matching the processor/parameter. If parameterIndexOrId >= 0, creates a new `ProcessorParameterTarget` with `sdouble` smoothing (configured via `SmoothingTime` in ms). On each value, the target applies smoothing via `sdouble::advance()`, clamps to 0..1, converts through `targetRange`, then calls `processor->setAttribute()`.
Pair with: `connectToMacroControl` (macro target alternative), `connectToGlobalModulator` (modulator source alternative)
Source:
  ScriptingApiObjects.cpp:9440  connectToModuleParameter() -> ProcessorHelpers::getFirstProcessorWithName() -> RangeHelpers::getDoubleRange() -> new ProcessorParameterTarget() -> Cable::addTarget()
  ScriptingApiObjects.cpp:9391  ProcessorParameterTarget::sendValue() -> sdouble::advance() -> targetRange.convertFrom0to1() -> processor->setAttribute()
