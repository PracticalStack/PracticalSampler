LorisManager::synthesise(ScriptObject file) -> Array

Thread safety: UNSAFE -- allocates heap buffers for audio output, DLL/library call for additive resynthesis.
Resynthesises audio from the analysed (and optionally processed) partial list.
Returns an array of Buffer objects (one per channel) at the original sample rate.
If partials were modified via process() or processCustom(), resynthesis reflects
those changes. Use process(file, "reset", {}) to revert before resynthesising.

Required setup:
  const var lm = Engine.getLorisManager();
  lm.analyse(audioFile, 440.0);

Dispatch/mechanics:
  initThreadController() -> LorisManager::synthesise()
    -> loris_synthesize() via C API
    -> returns VariantBuffer per channel

Pair with:
  analyse -- file must be analysed first
  process/processCustom -- modify partials before resynthesis

Anti-patterns:
  - Do NOT pass a non-File object -- silently returns an empty array.

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::synthesise()
    -> LorisManager::synthesise() -> loris_synthesize() via C API
