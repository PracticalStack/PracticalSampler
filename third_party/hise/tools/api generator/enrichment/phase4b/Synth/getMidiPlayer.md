Synth::getMidiPlayer(String playerId) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptedMidiPlayer wrapper on the heap. No objectsCanBeCreated() guard.
Returns a MidiPlayer handle to the named MIDI player processor. Uses global-rooted search
(entire module tree from getMainSynthChain), not just the parent synth's subtree.

Dispatch/mechanics:
  ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), playerId)
  -> two-step validation: checks existence, then checks dynamic_cast<MidiPlayer*>
  -> distinct error messages for "not found" vs "not a MIDI Player"

Anti-patterns:
  - Do NOT call at runtime -- allocates wrapper objects. Cache in onInit despite no
    formal onInit restriction.

Source:
  ScriptingApi.cpp  Synth::getMidiPlayer()
    -> ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), playerId)
    -> dynamic_cast<MidiPlayer*>
    -> wraps in new ScriptingObjects::ScriptedMidiPlayer
