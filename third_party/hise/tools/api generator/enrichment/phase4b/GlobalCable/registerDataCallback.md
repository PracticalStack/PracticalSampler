GlobalCable::registerDataCallback(Function dataCallbackFunction) -> undefined

Thread safety: UNSAFE -- allocates callback objects and modifies the cable target list
Registers a function to be called asynchronously when data is sent through the cable via `sendData()`. The callback receives the deserialised data (JSON, String, Array, Buffer, etc.). Operates on the data channel, independent of the value channel. A recursion guard prevents this reference's own data callback from firing when it sends data.
Callback signature: f(var data)
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");

inline function onData(data) { Console.print(data.noteNumber); };
cable.registerDataCallback(onData);
```
Dispatch/mechanics: Creates a `DataCallback` inner struct registered as a `CableTargetBase`. When `Cable::sendData()` fires, each `DataCallback` deserialises the binary stream via `var::readFromStream()` and calls `callback.call1()` (high-priority async via `WeakCallbackHolder`). The `dataRecursion` flag on the sending reference prevents its own callbacks from re-firing.
Pair with: `sendData` (sends data that triggers this callback), `deregisterCallback` (remove this callback)
Source:
  ScriptingApiObjects.cpp:9135  registerDataCallback() -> new DataCallback() -> Cable::addTarget()
  ScriptingApiObjects.cpp:9047  DataCallback::sendData() -> dataRecursion check -> var::readFromStream() -> callback.call1()
