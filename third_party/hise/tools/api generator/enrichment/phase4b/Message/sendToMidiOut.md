Message::sendToMidiOut() -> undefined

Thread safety: WARNING -- acquires SimpleReadWriteLock (ScopedWriteLock) in MainController::sendToMidiOut()
Forwards the current event to the plugin's MIDI output. Calls makeArtificial() internally
(MIDI output requires artificial events), then adds to the output MIDI buffer.

Anti-patterns:
  - EnableMidiOut project setting must be enabled. Backend builds report an error if not;
    frontend builds silently do nothing useful if MIDI output was not enabled at compile time.
  - Calling sendToMidiOut() makes the event artificial as a side effect. After this call,
    the event is artificial even if it was not before -- affects downstream event matching.
  - [BUG] No null check on messageHolder (not guarded by ENABLE_SCRIPTING_SAFE_CHECKS).
    Calling outside a mutable callback context dereferences null pointer.

Source:
  ScriptingApi.cpp  Message::sendToMidiOut()
    -> USE_BACKEND: checks EnableMidiOut project setting
    -> makeArtificial() (modifies event in-place)
    -> MainController::sendToMidiOut(*messageHolder) under ScopedWriteLock
