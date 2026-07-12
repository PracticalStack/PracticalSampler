Synth::getModulatorIndex(Integer chainId, String id) -> Integer

Thread safety: WARNING -- string comparison in iteration loop involves atomic ref-count operations. O(n) in chain length.
Returns the zero-based index of a modulator within the specified chain. chainId: 1 = GainModulation,
2 = PitchModulation. No onInit restriction -- callable at runtime.

Anti-patterns:
  - Do NOT use chainId 0 -- triggers jassertfalse and script error "No valid chainType".
    Valid: 1 (Gain), 2 (Pitch).

Pair with:
  setModulatorAttribute -- use the returned index to set attributes on chain modulators
  addModulator -- add a modulator whose index you can then query

Source:
  ScriptingApi.cpp  Synth::getModulatorIndex()
    -> switch(chainId): case GainModulation(1) -> owner->gainChain, case PitchModulation(2) -> owner->pitchChain
    -> iterates chain handler, matches by processor ID
