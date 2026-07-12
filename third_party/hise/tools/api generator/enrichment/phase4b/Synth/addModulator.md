Synth::addModulator(Integer chainId, String type, String id) -> ScriptObject

Thread safety: UNSAFE -- allocates a new processor via ModuleHandler.addModule, acquires ScopedTicket, kills voices, uses GlobalAsyncModuleHandler.
Dynamically adds a modulator to the parent synth's gain or pitch chain. Returns a ScriptModulator handle.
If a modulator with the same id already exists, the existing processor is returned.

Dispatch/mechanics:
  chainId: 1 = GainModulation (owner->gainChain), 2 = PitchModulation (owner->pitchChain)
  moduleHandler.addModule(chain, type, id, -1) -- always appends to end
  Thread-safe via KillStateHandler + GlobalAsyncModuleHandler

Pair with:
  removeModulator -- remove a previously added modulator
  getModulator -- retrieve an existing modulator by name
  getModulatorIndex / setModulatorAttribute -- index-based access to chain modulators

Anti-patterns:
  - Do NOT use chainId 0 -- produces script error "No valid chainType". Valid: 1 (Gain), 2 (Pitch).

Source:
  ScriptingApi.cpp  Synth::addModulator()
    -> switch(chainId): case GainModulation(1) / PitchModulation(2)
    -> moduleHandler.addModule(chain, type, id, -1)
