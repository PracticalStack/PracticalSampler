Synth::getRoutingMatrix(String processorId) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptRoutingMatrix wrapper on the heap. No objectsCanBeCreated() guard.
Returns a ScriptRoutingMatrix handle to configure multi-channel output routing for the named
processor. Uses global-rooted search (entire module tree). Two-step validation: checks existence,
then checks RoutableProcessor interface.

Anti-patterns:
  - Do NOT call at runtime -- allocates wrapper objects. Cache in onInit despite no
    formal onInit restriction.

Source:
  ScriptingApi.cpp  Synth::getRoutingMatrix()
    -> ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), processorId)
    -> dynamic_cast<RoutableProcessor*>
    -> wraps in new ScriptingObjects::ScriptRoutingMatrix
