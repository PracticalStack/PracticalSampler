ChildSynth::setModulationInitialValue(int chainIndex, float initialValue) -> undefined

Thread safety: UNSAFE -- calls ModulatorChain::setInitialValue which may trigger chain recalculation
Sets the initial modulation value for the specified modulator chain. This is the default
value the chain outputs when no modulators are active. Chain indices: 1 = Gain, 2 = Pitch.
Reports script error if chainIndex does not correspond to a valid ModulatorChain.
Dispatch/mechanics:
  getChildProcessor(chainIndex) cast to ModulatorChain*
    -> mc->setInitialValue(initialValue)
Pair with:
  getModulatorChain -- get chain handle for intensity/bypass control
  addModulator -- add modulators that override the initial value
Source:
  ScriptingApiObjects.cpp  setModulationInitialValue()
    -> dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex))
    -> mc->setInitialValue(initialValue)
