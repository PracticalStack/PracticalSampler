Engine::createMidiList() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a MidiList -- a fixed-size array of 128 integer values (one per MIDI note).
Optimized for MIDI lookups (key-to-velocity, note filtering, custom tables). Supports
fast Base64 serialization for preset storage. Values default to -1 (unused/disabled).
Source:
  ScriptingApi.cpp  Engine::createMidiList()
    -> new MidiList
