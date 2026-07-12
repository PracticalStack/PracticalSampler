MidiPlayer::getEventListFromSequence(Integer sequenceIndexOneBased) -> Array

Thread safety: UNSAFE -- Creates MessageHolder objects on the heap for each event.
Returns an array of MessageHolder objects for all MIDI events in the specified sequence's current track. Timestamps use samples (default) or ticks depending on setUseTimestampInTicks().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var events = mp.getEventListFromSequence(1);
```
Pair with: MidiPlayer.flushMessageListToSequence -- write events back to a specific sequence. MidiPlayer.getEventList -- shorthand for current sequence.
Anti-patterns: Index must be >= 1. Index 0 triggers a script error ("Nope. One based!!!").
Source:
  ScriptingApiObjects.cpp:6250  getEventListFromSequence() -> MidiPlayer::getSequenceWithIndex() -> HiseMidiSequence::getEventList()
