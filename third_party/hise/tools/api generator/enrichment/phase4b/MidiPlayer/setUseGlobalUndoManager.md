MidiPlayer::setUseGlobalUndoManager(Integer shouldUseGlobalUndoManager) -> undefined

Thread safety: UNSAFE
Enables or disables undo tracking for MIDI edit operations. When enabled, uses the global undo manager shared with Engine.undo(). Undo is disabled by default.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseGlobalUndoManager(true);
```
Dispatch/mechanics: Calls MidiPlayer::setExternalUndoManager() which DESTROYS the owned UndoManager. When true, attaches the global ControlUndoManager. When false, sets both ownedUndoManager and undoManager to nullptr.
Pair with: MidiPlayer.undo, MidiPlayer.redo -- require this to be enabled. MidiPlayer.flushMessageList -- edit operations become undoable when enabled.
Anti-patterns: Undo is disabled by default. Calling undo()/redo() without enabling throws "Undo is deactivated". Calling setUseGlobalUndoManager(false) after enabling destroys the internal undo manager permanently -- undo cannot be re-enabled via the internal manager, only via the global one by calling setUseGlobalUndoManager(true) again.
Source:
  ScriptingApiObjects.cpp:6250  setUseGlobalUndoManager() -> MidiPlayer::setExternalUndoManager() (destroys ownedUndoManager)
