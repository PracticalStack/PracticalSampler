GlobalRoutingManager::getCable(String cableId) -> ScriptObject

Thread safety: UNSAFE -- allocates a new GlobalCableReference wrapper object on each call.
Returns a GlobalCable reference for the given cable ID. Creates the cable on demand if it
does not exist. Multiple calls with the same ID return separate wrappers sharing one cable.
Cable IDs starting with / are OSC-addressable; other IDs work only for internal routing.

Required setup:
  const var rm = Engine.getGlobalRoutingManager();

Pair with:
  GlobalCable.setValue/setValueNormalised -- write values through the cable
  GlobalCable.registerCallback -- observe value changes
  connectToOSC -- enable OSC routing for /-prefixed cable IDs

Anti-patterns:
  - Do NOT use cable IDs without / prefix if you need OSC routing -- non-prefixed cables
    are invisible to the OSC subsystem
  - Do NOT call getCable repeatedly for the same ID expecting a cached wrapper -- each call
    allocates a new wrapper object. Store the result in a const var.

Source:
  ScriptingApiObjects.cpp:8920  GlobalRoutingManagerReference constructor
    -> Helpers::getOrCreate(mc) obtains singleton
  ScriptingApi.cpp:2506  Engine::getGlobalRoutingManager()
    -> new GlobalRoutingManagerReference(sp)
