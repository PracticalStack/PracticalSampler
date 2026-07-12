Engine::getFilterModeList() -> ScriptObject

Thread safety: UNSAFE -- creates new FilterModeObject on heap (18 integer constants)
Returns a FilterModes object with named constants for all filter types:
LowPass, HighPass, LowShelf, HighShelf, Peak, ResoLow, StateVariableLP,
StateVariableHP, MoogLP, OnePoleLowPass, OnePoleHighPass, StateVariablePeak,
StateVariableNotch, StateVariableBandPass, Allpass, LadderFourPoleLP,
LadderFourPoleHP, RingMod.
Source:
  ScriptingApi.cpp  Engine::getFilterModeList()
    -> new FilterModeObject with addConstant() for each mode
