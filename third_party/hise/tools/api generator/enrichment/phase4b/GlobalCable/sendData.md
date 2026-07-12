GlobalCable::sendData(NotUndefined dataToSend) -> undefined

Thread safety: UNSAFE -- allocates a MemoryOutputStream on the heap; must not be called from the audio thread or a synchronous cable callback
Serialises any HISEScript value (JSON, strings, arrays, buffers) to a binary stream and sends it to all cable targets with registered data callbacks. Independent of the value channel. A recursion guard prevents this reference's own data callbacks from firing.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.sendData({"noteNumber": 60, "velocity": 100});
```
Dispatch/mechanics: Creates a `MemoryOutputStream`, serialises the var via `writeToStream()`, sets `dataRecursion = true`, then calls `Cable::sendData(nullptr, data, size)` which iterates all targets calling `target->sendData()`. Each `DataCallback` deserialises the binary stream back to a var.
Pair with: `registerDataCallback` (register receiver for data sent here)
Anti-patterns: Do NOT call from the audio thread or from inside a synchronous cable callback -- heap allocation makes it unsafe. Large objects incur serialisation cost on every send.
Source:
  ScriptingApiObjects.cpp:9013  sendData() -> MemoryOutputStream -> var::writeToStream() -> Cable::sendData()
  GlobalRoutingManager.cpp:1725  Cable::sendData() -> target->sendData()
