Synth::removeEffect(ScriptObject effect) -> Integer

Thread safety: UNSAFE -- delegates to ModuleHandler::removeModule which checks thread, schedules async removal via GlobalAsyncModuleHandler.
Removes a previously added effect from the parent synth's effect chain. Returns true if removal
was scheduled, false if the parameter was not a ScriptEffect. Removal is asynchronous.

Dispatch/mechanics:
  dynamic_cast<ScriptingEffect*>(effect) check
  -> ModuleHandler::removeModule()
    -> if audio thread: throws exception
    -> if null processor: returns true (no-op)
    -> otherwise: GlobalAsyncModuleHandler::removeAsync()

Anti-patterns:
  - Do NOT call from the audio thread -- throws "Effects can't be removed from the audio thread!".
  - The removal is async -- getEffect/getAllEffects may still find the processor briefly after.
  - Passing a non-ScriptEffect object silently returns false with no error.

Pair with:
  addEffect -- add effects to the chain
  getEffect -- retrieve effect handles

Source:
  ScriptingApi.cpp  Synth::removeEffect()
    -> ModuleHandler::removeModule(processor)
