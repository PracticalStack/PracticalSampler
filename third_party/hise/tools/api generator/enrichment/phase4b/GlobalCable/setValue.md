GlobalCable::setValue(Double inputWithinRange) -> undefined

Thread safety: SAFE
Converts the input value from the local input range to normalised 0..1, then sends the normalised value to all cable targets. Without a range configured, behaves identically to `setValueNormalised()`.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setRange(20.0, 20000.0);
cable.setValue(440.0);
```
Dispatch/mechanics: Calls `inputRange.convertTo0to1()` on the input, then delegates to `setValueNormalised()` which calls `Cable::sendValue(nullptr, v)`. The cable clamps to 0..1 and iterates all targets.
Pair with: `getValue` (write/read pair), `setValueNormalised` (skip range conversion), `setRange`/`setRangeWithSkew`/`setRangeWithStep` (configure the input range)
Anti-patterns: Calling `setValue()` without first configuring a range is equivalent to `setValueNormalised()` -- if you intend a mapped range, call `setRange()` first.
Source:
  ScriptingApiObjects.cpp:9007  setValue() -> inputRange.convertTo0to1() -> setValueNormalised() -> Cable::sendValue()
