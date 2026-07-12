Modulator::addModulator(Integer chainIndex, String typeName, String modName) -> ScriptObject

Thread safety: UNSAFE -- creates a new module via ModuleHandler, coordinates
with KillStateHandler, heap allocates the wrapper.
Adds a new child modulator to one of this modulator's internal chains. Returns
a Modulator handle for the newly created modulator, or undefined on failure.
Reports a script error if the chain index is invalid.

Required setup:
  const var env = Synth.getModulator("GainModulation1");

Dispatch/mechanics:
  getChildProcessor(chainIndex) -> cast to ModulatorChain*
    -> moduleHandler.addModule(chain, typeName, modName, -1)
    -> wraps result in new ScriptingModulator

Pair with:
  getModulatorChain -- access the chain to inspect or add more modulators
  Synth.removeModulator -- remove a dynamically added modulator

Source:
  ScriptingApiObjects.cpp:3239  addModulator()
    -> ModulatorChain* c = getChildProcessor(chainIndex)
    -> moduleHandler.addModule(c, typeName, modName, -1)
