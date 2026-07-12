MidiPlayer::undo() -> undefined

Thread safety: UNSAFE -- Calls UndoManager::undo() which replays actions on the heap.
Undoes the last MIDI edit operation (flushMessageList, setTimeSignature, etc.). Throws a script error if undo is deactivated. Does nothing if no sequence is loaded.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseGlobalUndoManager(true);
// ... perform edits ...
mp.undo();
```
Pair with: MidiPlayer.redo -- redo the undone edit. MidiPlayer.setUseGlobalUndoManager -- must be enabled first. MidiPlayer.flushMessageList -- creates undoable edit actions.
Anti-patterns: Calling undo() without first enabling the undo manager via setUseGlobalUndoManager(true) throws "Undo is deactivated".
Source:
  ScriptingApiObjects.cpp:6250  undo() -> UndoManager::undo()
