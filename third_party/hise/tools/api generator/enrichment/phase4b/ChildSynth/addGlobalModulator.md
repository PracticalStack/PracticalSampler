ChildSynth::addGlobalModulator(var chainIndex, var globalMod, String modName) -> ScriptingModulator

Thread safety: UNSAFE -- modifies module tree, creates objects, acquires locks via ModuleHandler
Adds a per-voice global modulator receiver to the specified modulator chain. The receiver
tracks a global modulator source from a GlobalModulatorContainer, providing per-voice values.
Required setup:
  const var cs = Synth.getChildSynth("MySynth");
  const var globalLfo = Synth.getModulator("GlobalLFO"); // must be inside GlobalModulatorContainer
Dispatch/mechanics:
  dynamic_cast to ModulatorChain* via getChildProcessor(chainIndex)
    -> moduleHandler.addAndConnectToGlobalModulator(chain, globalMod, modName, false)
    -> returns new ScriptingModulator wrapping the created receiver
Pair with:
  addStaticGlobalModulator -- single-value-per-block variant (more CPU-efficient)
  getModulatorChain -- get chain handle before/after adding modulators
Anti-patterns:
  - Do NOT pass a modulator that is not inside a GlobalModulatorContainer -- silently
    returns undefined without error if dynamic_cast to ScriptingModulator fails
  - Do NOT use chainIndex 0 -- that is MidiProcessor, not a modulator chain. Use 1 (Gain) or 2 (Pitch)
Source:
  ScriptingApiObjects.cpp:4218  ScriptingSynth constructor registers method
  ScriptingApiObjects.cpp  addGlobalModulator()
    -> getChildProcessor(chainIndex) cast to ModulatorChain*
    -> moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName)
