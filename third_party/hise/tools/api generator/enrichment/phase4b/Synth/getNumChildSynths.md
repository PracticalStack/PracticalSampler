Synth::getNumChildSynths() -> Integer

Thread safety: SAFE -- calls getHandler()->getNumProcessors() which returns a stored count, no allocations.
Returns the number of child sound generators. Parent synth must be a Chain type
(SynthChain or SynthGroup). Reports script error if called on a non-Chain synth.

Anti-patterns:
  - Do NOT call on a standalone ModulatorSynth that is not a SynthChain or SynthGroup --
    produces script error "getNumChildSynths() can only be called on Chains!".

Pair with:
  getChildSynth / getChildSynthByIndex -- retrieve specific child synths

Source:
  ScriptingApi.cpp  Synth::getNumChildSynths()
    -> dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors()
