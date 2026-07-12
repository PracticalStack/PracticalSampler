Synth::getChildSynth(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only).
Returns a ChildSynth handle to the named child sound generator within the parent synth's
subtree. Uses owner-rooted search. Parent must be a container type (SynthChain/SynthGroup).

Pair with:
  getChildSynthByIndex -- positional access instead of name-based
  getNumChildSynths -- determine valid index range

Source:
  ScriptingApi.cpp  Synth::getChildSynth()
    -> Processor::Iterator<ModulatorSynth>(owner)
    -> matches by processor ID
    -> wraps in new ScriptingObjects::ScriptingSynth
