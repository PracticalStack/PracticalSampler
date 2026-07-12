Synth::setMacroControl(Integer macroIndex, Double newValue) -> undefined

Thread safety: UNSAFE -- calls ModulatorSynthChain::setMacroControl() with sendNotification, dispatches to connected parameters.
Sets one of the eight macro controllers (1-8) to a new value (0.0-127.0). Only works when the
parent synth is a ModulatorSynthChain -- reports error otherwise.

Anti-patterns:
  - macroIndex is 1-based (1-8), NOT 0-based. Passing 0 triggers error "macroIndex must be
    between 1 and 8!".
  - Only works on ModulatorSynthChain. Calling from a non-chain synth reports "setMacroControl()
    can only be called on ModulatorSynthChains".

Source:
  ScriptingApi.cpp  Synth::setMacroControl()
    -> dynamic_cast<ModulatorSynthChain*>(owner)
    -> chain->setMacroControl(macroIndex - 1, newValue, sendNotification)
