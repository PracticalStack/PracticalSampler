GlobalRoutingManager::removeOSCCallback(String oscSubAddress) -> Integer

Thread safety: UNSAFE -- modifies the callback list (array removal).
Removes the first OSC callback registered for the given sub-address. Returns true if a
callback was found and removed, false if no callback matches.

Required setup:
  const var rm = Engine.getGlobalRoutingManager();

Pair with:
  addOSCCallback -- register the callback that this method removes

Anti-patterns:
  - Only removes the first matching callback -- if multiple callbacks were registered for
    the same sub-address, call repeatedly until it returns false to remove all of them

Source:
  ScriptingApiObjects.cpp:9095  removeOSCCallback()
    -> iterates callbacks, removes first match by subDomain
    -> returns true if found
