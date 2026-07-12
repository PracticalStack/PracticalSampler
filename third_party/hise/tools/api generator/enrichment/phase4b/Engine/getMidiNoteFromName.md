Engine::getMidiNoteFromName(String midiNoteName) -> Integer

Thread safety: WARNING -- string comparisons (O(128) linear search)
Converts a note name (e.g., "C3") to MIDI note number. Returns -1 if not found.
Middle C = "C3" = note 60.
Anti-patterns:
  - Do NOT call in tight audio-thread loops -- O(128) string comparisons per call
Source:
  ScriptingApi.cpp  Engine::getMidiNoteFromName()
    -> iterates 0-127 comparing getMidiNoteName(i) to input
