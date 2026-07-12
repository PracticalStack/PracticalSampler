GlobalRoutingManager::addOSCCallback(String oscSubAddress, Function callback) -> undefined

Thread safety: UNSAFE -- allocates an OSCCallback object and modifies the callback list.
Registers a callback for incoming OSC messages matching the given sub-address. The
sub-address is combined with the connection domain to form the full OSC address pattern.
Can be called before or after connectToOSC -- order does not matter.
Callback signature: callback(String subAddress, var value)

Required setup:
  const var rm = Engine.getGlobalRoutingManager();

Dispatch/mechanics:
  Creates OSCCallback with WeakCallbackHolder set to high priority
    -> if connectToOSC already called, rebuilds full address immediately
    -> callback executes on the OSC receiver thread (not deferred to UI thread)
    -> multi-arg OSC messages pass an Array as the value parameter

Pair with:
  removeOSCCallback -- unregister when no longer needed
  connectToOSC -- must be called (before or after) to establish the OSC connection

Anti-patterns:
  - Do NOT perform heavy processing or non-thread-safe UI mutations in the callback --
    it executes on the OSC receiver thread, not the scripting UI thread
  - For multi-argument OSC messages, the value parameter is an Array, not a single value --
    check the type before processing

Source:
  ScriptingApiObjects.cpp:9112  addOSCCallback()
    -> new OSCCallback(this, oscSubAddress, callback)
    -> callback.setHighPriority() -- executes on OSC receiver thread
  ScriptingApiObjects.cpp:8975  oscMessageReceived()
    -> iterates callbacks, fires matching fullAddress patterns
