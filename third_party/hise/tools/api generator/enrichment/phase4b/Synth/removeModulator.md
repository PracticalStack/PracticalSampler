Synth::removeModulator(ScriptObject mod) -> Integer

Thread safety: UNSAFE -- delegates to ModuleHandler::removeModule, schedules async removal.
Removes a previously added modulator from the parent synth's chain. Returns true if removal
was scheduled, false if the parameter was not a ScriptModulator. Removal is asynchronous.

Anti-patterns:
  - Do NOT call from the audio thread -- throws "Effects can't be removed from the audio thread!"
    (error message says "Effects" even for modulators -- shared removeModule implementation).
  - Passing a non-ScriptModulator object silently returns false.

Pair with:
  addModulator -- add modulators to the chain
  getModulator -- retrieve modulator handles

Source:
  ScriptingApi.cpp  Synth::removeModulator()
    -> ModuleHandler::removeModule(processor)
