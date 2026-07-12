ChildSynth::addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName) -> ScriptingModulator

Thread safety: UNSAFE -- modifies module tree, creates objects, acquires locks via ModuleHandler
Adds a static global modulator receiver to the specified modulator chain. Unlike
addGlobalModulator (per-voice), static provides a single value per audio block.
More CPU-efficient when per-voice resolution is not needed.
Required setup:
  const var cs = Synth.getChildSynth("MySynth");
  const var globalLfo = Synth.getModulator("GlobalLFO"); // must be inside GlobalModulatorContainer
Dispatch/mechanics:
  dynamic_cast to ModulatorChain* via getChildProcessor(chainIndex)
    -> moduleHandler.addAndConnectToGlobalModulator(chain, mod, modName, true)
    -> the true flag selects static (time-variant, not voice-variant) connection
Pair with:
  addGlobalModulator -- per-voice variant when voice independence is needed
  getModulatorChain -- get chain handle before/after adding modulators
Anti-patterns:
  - Do NOT pass a modulator not inside a GlobalModulatorContainer -- silently
    returns undefined without error if dynamic_cast fails
  - Do NOT use chainIndex 0 -- MidiProcessor, not a modulator chain
Source:
  ScriptingApiObjects.cpp  addStaticGlobalModulator()
    -> getChildProcessor(chainIndex) cast to ModulatorChain*
    -> moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName, true)
