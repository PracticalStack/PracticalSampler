Effect::getModulatorChain(Integer chainIndex) -> ScriptObject

Thread safety: UNSAFE -- allocates a new ScriptingModulator wrapper on the heap.
Returns a Modulator handle to the modulator chain at the specified child
processor index. Reports a script error if no modulator chain exists at
the given index.
Pair with:
  addModulator -- add a new modulator to the chain
Source:
  ScriptingApiObjects.cpp:3373  ScriptingEffect::getModulatorChain()
    -> dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex))
    -> wraps in ScriptingModulator handle
