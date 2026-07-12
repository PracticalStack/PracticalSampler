Synth::getWavetableController(String processorId) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptWavetableController wrapper on the heap. No objectsCanBeCreated() guard.
Returns a ScriptWavetableController handle for the named WavetableSynth. Uses global-rooted
search (entire module tree). Provides access to wavetable position, gain table, etc.

Anti-patterns:
  - Do NOT call at runtime -- allocates wrapper objects. Cache in onInit.
  - Error message when processor is not a WavetableSynth incorrectly says "does not have
    a routing matrix" (copy-paste bug from getRoutingMatrix).

Source:
  ScriptingApi.cpp  Synth::getWavetableController()
    -> ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), processorId)
    -> dynamic_cast<WavetableSynth*>
    -> wraps in ScriptWavetableController
