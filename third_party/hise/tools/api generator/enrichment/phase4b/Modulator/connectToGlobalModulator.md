Modulator::connectToGlobalModulator(String globalModulationContainerId, String modulatorId) -> Integer

Thread safety: UNSAFE -- involves string concatenation and global modulator
connection logic with potential message dispatching.
Connects a global receiver modulator to a source modulator inside a
GlobalModulatorContainer. Only works on global receiver types
(GlobalVoiceStartModulator, GlobalTimeVariantModulator,
GlobalStaticTimeVariantModulator, GlobalEnvelopeModulator).

Dispatch/mechanics:
  dynamic_cast<GlobalModulator*>(mod)
    -> gm->connectToGlobalModulator(containerId + ":" + modulatorId)
  Reports script error if cast fails (not a global receiver type)

Anti-patterns:
  - Do NOT call on regular modulators (LFO, AHDSR, etc.) -- only works on global
    receiver types. Reports script error otherwise.

Source:
  ScriptingApiObjects.cpp:2993  connectToGlobalModulator()
    -> casts to GlobalModulator* -> connectToGlobalModulator(containerId:modulatorId)
