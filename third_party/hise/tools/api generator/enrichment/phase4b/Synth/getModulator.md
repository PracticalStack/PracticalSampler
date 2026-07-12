Synth::getModulator(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only).
Returns a ScriptModulator handle to the named modulator within the parent synth's subtree.
Uses owner-rooted search. Finds all modulator types: LFOs, envelopes, voice start modulators, etc.

Pair with:
  getAllModulators -- get multiple modulators by wildcard (but searches entire tree)
  addModulator / removeModulator -- dynamic modulator chain modification
  getModulatorIndex / setModulatorAttribute -- index-based chain access

Source:
  ScriptingApi.cpp  Synth::getModulator()
    -> Processor::Iterator<Modulator>(owner)
    -> matches by processor ID
    -> wraps in new ScriptingObjects::ScriptingModulator
