ChildSynth::getModulatorChain(var chainIndex) -> ScriptingModulator

Thread safety: UNSAFE -- creates a new ScriptingModulator wrapper object (heap allocation)
Returns a Modulator reference to the modulator chain at the specified index. The chain
itself is a Modulator, so the returned handle controls chain intensity and bypass state.
Chain indices: 1 = GainModulation, 2 = PitchModulation.
Dispatch/mechanics:
  getChildProcessor(chainIndex) cast to Modulator* (not ModulatorChain*)
    -> returns new ScriptingModulator wrapping the chain-as-modulator
Pair with:
  addModulator -- add modulators to this chain
  addGlobalModulator / addStaticGlobalModulator -- connect global sources
  setModulationInitialValue -- set default value when no modulators active
Anti-patterns:
  - Do NOT pass chainIndex 0 (MidiProcessor) -- may succeed the Modulator cast but gives
    a handle to the wrong chain type. Error only triggers when cast fails completely
Source:
  ScriptingApiObjects.cpp  getModulatorChain()
    -> synth->getChildProcessor(chainIndex) cast to Modulator*
    -> returns new ScriptingModulator(getScriptProcessor(), m)
