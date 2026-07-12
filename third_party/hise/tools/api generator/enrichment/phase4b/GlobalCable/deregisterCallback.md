GlobalCable::deregisterCallback(Function callbackFunction) -> Integer

Thread safety: UNSAFE -- modifies the cable target list and deallocates callback objects
Removes a previously registered callback (value or data) from this cable reference. Searches both data callbacks and value callbacks. Returns true if found and removed, false if not found.
Required setup:
```
const var cable = Engine.getGlobalRoutingManager().getCable("id");
cable.deregisterCallback(onValue);
```
Dispatch/mechanics: Iterates `dataCallbacks` then `callbacks` arrays, using `WeakCallbackHolder::matches()` to find the matching function reference. Removes the first match and returns true. The removed callback's destructor calls `Cable::removeTarget()`.
Pair with: `registerCallback` (register value callback), `registerDataCallback` (register data callback)
Source:
  ScriptingApiObjects.cpp:9272  deregisterCallback() -> WeakCallbackHolder::matches() -> OwnedArray::removeObject() -> ~Callback/~DataCallback -> Cable::removeTarget()
