Synth::getMidiProcessor(String name) -> ScriptObject

Thread safety: INIT -- checks objectsCanBeCreated() (onInit only).
Returns a ScriptMidiProcessor handle to the named MIDI processor within the parent synth's
subtree. Uses owner-rooted search. Has a self-exclusion check -- you cannot get a reference
to the script processor that owns this Synth object.

Anti-patterns:
  - Do NOT pass the current script processor's own ID -- produces error "You can't get a
    reference to yourself!".

Source:
  ScriptingApi.cpp  Synth::getMidiProcessor()
    -> self-exclusion check: name == getProcessor()->getId()
    -> Processor::Iterator<MidiProcessor>(owner)
    -> wraps in new ScriptingObjects::ScriptingMidiProcessor
