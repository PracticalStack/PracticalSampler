GlobalCable::getValue() -> Double

Thread safety: SAFE
Returns the current cable value converted from internal normalised range (0..1) back through the local input range. If no range set, behaves identically to `getValueNormalised()`.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRange(0.0, 100.0);
var v = cable.getValue();
```
Dispatch/mechanics: Calls `getValueNormalised()` to read `Cable::lastValue`, then applies `inputRange.convertFrom0to1()` to map back to the configured range.
Pair with: `setValue` (write/read pair), `getValueNormalised` (raw vs range-converted read), `setRange` (configures the conversion)
Source:
  ScriptingApiObjects.cpp:8987  getValue() -> getValueNormalised() -> inputRange.convertFrom0to1()
