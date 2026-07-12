MidiPlayer::flushMessageList(Array messageList) -> undefined

Thread safety: UNSAFE -- Allocates EditAction, writes to sequence. Undoable operation.
Writes the given array of MessageHolder objects into the current sequence, replacing existing MIDI data on the current track. Undoable if setUseGlobalUndoManager(true) was called. Timestamps are interpreted as samples (default) or ticks depending on setUseTimestampInTicks().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.flushMessageList(events);
```
Dispatch/mechanics: Delegates to flushMessageListToSequence() with the current sequence index. Creates an EditAction that captures old/new events. If undoManager is set, performs via undo manager; otherwise performs directly without undo tracking.
Pair with: MidiPlayer.getEventList -- read events before editing. MidiPlayer.undo -- revert edits. MidiPlayer.setUseTimestampInTicks -- controls timestamp format.
Source:
  ScriptingApiObjects.cpp:6250  flushMessageList() -> flushMessageListToSequence(-1) -> MidiPlayer::flushEdit() -> EditAction::perform()
