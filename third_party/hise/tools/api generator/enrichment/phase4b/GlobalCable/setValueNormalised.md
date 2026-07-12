GlobalCable::setValueNormalised(Double normalisedInput) -> undefined

Thread safety: SAFE
Sends a normalised value (0..1) directly to all cable targets, bypassing the local input range. The value is clamped to 0..1 internally.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.setValueNormalised(0.5);
```
Dispatch/mechanics: Calls `Cable::sendValue(nullptr, v)` which stores `jlimit(0.0, 1.0, v)` as `lastValue`, then iterates all targets calling `target->sendValue(lastValue)`.
Pair with: `getValueNormalised` (write/read pair), `setValue` (range-converted alternative)
Source:
  ScriptingApiObjects.cpp:9001  setValueNormalised() -> Cable::sendValue(nullptr, v)
  GlobalRoutingManager.cpp:1738  Cable::sendValue() -> jlimit() -> target->sendValue()
