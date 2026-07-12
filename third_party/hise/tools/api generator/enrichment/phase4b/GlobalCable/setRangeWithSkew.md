GlobalCable::setRangeWithSkew(Double min, Double max, Double midPoint) -> undefined

Thread safety: SAFE
Sets the local input range with a skew factor derived from a mid point. The `midPoint` specifies which input value maps to 0.5 in normalised space, creating a logarithmic or exponential curve. Useful for frequency or gain ranges.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRangeWithSkew(20.0, 20000.0, 1000.0);
```
Pair with: `setRange` (linear alternative), `setRangeWithStep` (quantised alternative), `setValue`/`getValue` (use the configured range)
Source:
  ScriptingApiObjects.cpp:9033  setRangeWithSkew() -> InvertableParameterRange(min, max) -> setSkewForCentre(midPoint) -> checkIfIdentity()
