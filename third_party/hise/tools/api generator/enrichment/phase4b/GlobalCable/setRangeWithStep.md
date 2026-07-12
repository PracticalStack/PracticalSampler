GlobalCable::setRangeWithStep(Double min, Double max, Double stepSize) -> undefined

Thread safety: SAFE
Sets the local input range with a step size for quantised values. The output of `getValue()` will snap to multiples of the step size within the range.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRangeWithStep(0.0, 127.0, 1.0);
```
Pair with: `setRange` (no quantisation), `setRangeWithSkew` (skewed alternative), `setValue`/`getValue` (use the configured range)
Source:
  ScriptingApiObjects.cpp:9040  setRangeWithStep() -> InvertableParameterRange(min, max, stepSize) -> checkIfIdentity()
