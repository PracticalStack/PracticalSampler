ChildSynth::addModulator(var chainIndex, var typeName, var modName) -> ScriptingModulator

Thread safety: UNSAFE -- modifies module tree, acquires locks via ModuleHandler
Creates and adds a new modulator of the specified type to the given modulator chain.
The typeName must be the exact C++ class name (e.g., "LFOModulator", "Velocity", "ConstantModulator").
Required setup:
  const var cs = Synth.getChildSynth("MySynth");
Dispatch/mechanics:
  dynamic_cast to ModulatorChain* via getChildProcessor(chainIndex)
    -> moduleHandler.addModule(chain, typeName, modName, -1)
    -> returns new ScriptingModulator wrapping the created modulator
Pair with:
  getModulatorChain -- get chain handle to control chain intensity/bypass
  addGlobalModulator / addStaticGlobalModulator -- for connecting to global sources instead
Anti-patterns:
  - Do NOT use human-readable type names ("LFO") -- must use exact C++ class names
    ("LFOModulator"). Silent failure, returns undefined with no error
  - Do NOT use chainIndex 0 -- that is MidiProcessor. Use 1 (Gain) or 2 (Pitch)
Source:
  ScriptingApiObjects.cpp  addModulator()
    -> getChildProcessor(chainIndex) cast to ModulatorChain*
    -> moduleHandler.addModule(c, typeName, modName, -1)
