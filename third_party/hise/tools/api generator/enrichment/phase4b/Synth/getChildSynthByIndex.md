Synth::getChildSynthByIndex(Integer index) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only).
Returns a ChildSynth handle at the given index in the parent synth's child list. Parent
must be a Chain type (SynthChain/SynthGroup). Use getNumChildSynths() for valid range.

Anti-patterns:
  - Do NOT assume an error is reported for invalid index or non-Chain parent -- the method
    silently returns an invalid (null-wrapped) handle. Methods called on it will produce
    confusing errors.

Pair with:
  getChildSynth -- name-based lookup alternative
  getNumChildSynths -- get the valid index range

Source:
  ScriptingApi.cpp  Synth::getChildSynthByIndex()
    -> dynamic_cast<Chain*>(owner)->getHandler()->getProcessor(index)
    -> wraps in new ScriptingObjects::ScriptingSynth
