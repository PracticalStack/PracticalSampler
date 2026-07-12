MidiPlayer::flushMessageListToSequence(Array messageList, Integer sequenceIndexOneBased) -> undefined

Thread safety: UNSAFE -- Allocates EditAction, writes to sequence. Undoable operation.
Writes the given array of MessageHolder objects into the sequence at the specified one-based index, replacing existing MIDI data on the current track. Undoable if setUseGlobalUndoManager(true) was called. Timestamps are interpreted as samples or ticks depending on setUseTimestampInTicks().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.flushMessageListToSequence(events, 1);
```
Dispatch/mechanics: Creates EditAction with old/new events, performs via undo manager if available, otherwise performs directly. Sends sequence update notification after writing.
Pair with: MidiPlayer.getEventListFromSequence -- read events from a specific sequence. MidiPlayer.flushMessageList -- shorthand for current sequence.
Anti-patterns: sequenceIndexOneBased must be >= 1. Invalid index triggers a script error.
Source:
  ScriptingApiObjects.cpp:6250  flushMessageListToSequence() -> MidiPlayer::flushEdit() -> EditAction::perform() -> writeArrayToSequence()
