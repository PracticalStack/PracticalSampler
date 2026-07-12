GlobalCable::setRange(Double min, Double max) -> undefined

Thread safety: SAFE
Sets the local input range for this cable reference using min/max (linear, no step size or skew). Used by `setValue()`/`getValue()` and registered value callbacks to convert between user-facing values and normalised 0..1 space. Does not affect `setValueNormalised()` or `getValueNormalised()`.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRange(0.0, 100.0);
```
Pair with: `setRangeWithSkew` (adds skew), `setRangeWithStep` (adds quantisation), `setValue`/`getValue` (use the configured range)
Source:
  ScriptingApiObjects.cpp:9027  setRange() -> InvertableParameterRange(min, max) -> checkIfIdentity()
