Engine::getGlobalRoutingManager() -> ScriptObject

Thread safety: UNSAFE -- creates new wrapper object on heap each call
Returns a GlobalRoutingManager reference for global cables and OSC. Each call creates
a new wrapper but all reference the same underlying singleton. Store in const var.
Pair with:
  GlobalRoutingManager.getCable -- get cable references
  GlobalRoutingManager.connectToOSC -- OSC communication
Source:
  ScriptingApi.cpp  Engine::getGlobalRoutingManager()
    -> new GlobalRoutingManagerReference
