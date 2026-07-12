Synth::createBuilder() -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptBuilder object on the heap.
Creates and returns a Builder object for programmatic module tree construction. The Builder
provides methods to create, configure, and connect processors without using the HISE IDE.

Dispatch/mechanics:
  new ScriptingObjects::ScriptBuilder(getScriptProcessor())
  Typical workflow: builder.clear() -> builder.create() -> configure -> builder.flush()

Source:
  ScriptingApi.cpp  Synth::createBuilder()
    -> var(new ScriptingObjects::ScriptBuilder(getScriptProcessor()))
