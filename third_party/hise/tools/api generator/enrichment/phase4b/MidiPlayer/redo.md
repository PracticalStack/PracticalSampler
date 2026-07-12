MidiPlayer::redo() -> undefined

Thread safety: UNSAFE -- Calls UndoManager::redo() which replays actions on the heap.
Redoes the last undone edit operation. Throws a script error if undo is deactivated (call setUseGlobalUndoManager(true) first). Does nothing if no sequence is loaded.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseGlobalUndoManager(true);
// ... edit, undo, then:
mp.redo();
```
Pair with: MidiPlayer.undo -- undo the last edit. MidiPlayer.setUseGlobalUndoManager -- must be enabled first.
Anti-patterns: Calling redo() without first enabling the undo manager via setUseGlobalUndoManager(true) throws "Undo is deactivated".
Source:
  ScriptingApiObjects.cpp:6250  redo() -> UndoManager::redo()
