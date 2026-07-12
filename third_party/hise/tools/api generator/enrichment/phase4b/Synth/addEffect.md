Synth::addEffect(String type, String id, Integer index) -> ScriptObject

Thread safety: UNSAFE -- allocates a new processor via ModuleHandler.addModule, acquires ScopedTicket, kills voices, uses GlobalAsyncModuleHandler.
Dynamically adds an effect to the parent synth's effect chain. Returns a ScriptEffect handle.
If an effect with the same id already exists, the existing processor is returned instead.

Dispatch/mechanics:
  moduleHandler.addModule(owner->effectChain, type, id, index)
    -> KillStateHandler suspends audio, allocates processor via factory
    -> GlobalAsyncModuleHandler manages insertion
  index = -1 appends to end; >= 0 inserts before that position

Pair with:
  removeEffect -- remove a previously added effect
  getEffect -- retrieve an existing effect by name

Anti-patterns:
  - Do NOT pass an invalid type name -- throws script error "Module with type X could not
    be generated". Use Synth.ModuleIds to discover available types.

Source:
  ScriptingApi.cpp  Synth::addEffect()
    -> moduleHandler.addModule(owner->effectChain, type, id, index)
