GlobalCable::getValueNormalised() -> Double

Thread safety: SAFE
Returns the raw normalised cable value (0.0--1.0), bypassing the local input range. Reads `Cable::lastValue` directly. Returns 0.0 if the cable reference is invalid.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
var v = cable.getValueNormalised();
```
Pair with: `setValueNormalised` (write/read pair), `getValue` (range-converted alternative)
Source:
  ScriptingApiObjects.cpp:8993  getValueNormalised() -> getCableFromVar() -> Cable::lastValue
