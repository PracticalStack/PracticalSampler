ChildSynth::getRoutingMatrix() -> ScriptRoutingMatrix

Thread safety: UNSAFE -- creates a new ScriptRoutingMatrix wrapper object (heap allocation)
Returns a RoutingMatrix handle for this synth's channel routing configuration.
Allows querying and modifying output channel assignments.
Anti-patterns:
  - Do NOT call on an invalid ChildSynth -- does not call checkValidObject() before
    creating the wrapper. Creates a RoutingMatrix wrapping nullptr, which will fail
    on subsequent calls
Source:
  ScriptingApiObjects.cpp  getRoutingMatrix()
    -> new ScriptRoutingMatrix(getScriptProcessor(), synth.get())
