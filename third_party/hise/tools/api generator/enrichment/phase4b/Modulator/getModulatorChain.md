Modulator::getModulatorChain(Integer chainIndex) -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptingModulator wrapper on the heap.
Returns a Modulator handle for the internal modulator chain at the given index.
ModulatorChain inherits from Modulator (via EnvelopeModulator), so the returned
handle can itself have attributes set, be bypassed, and have child modulators
added. Reports a script error if the chain index is invalid.

Dispatch/mechanics:
  getChildProcessor(chainIndex) -> cast to Modulator*
    -> wraps in new ScriptingModulator

Pair with:
  addModulator -- add child modulators to the returned chain
  getCurrentLevel -- poll the chain's output for UI display

Source:
  ScriptingApiObjects.cpp:3261  getModulatorChain()
    -> getChildProcessor(chainIndex) -> new ScriptingModulator wrapper
