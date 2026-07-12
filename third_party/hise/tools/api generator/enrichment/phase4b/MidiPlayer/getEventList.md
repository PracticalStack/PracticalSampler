MidiPlayer::getEventList() -> Array

Thread safety: UNSAFE -- Creates MessageHolder objects on the heap for each event.
Returns an array of MessageHolder objects for all MIDI events in the current sequence's current track. Timestamps use samples (default) or ticks depending on setUseTimestampInTicks(). Internally delegates to getEventListFromSequence(-1).
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var events = mp.getEventList();
```
Pair with: MidiPlayer.flushMessageList -- write modified events back. MidiPlayer.setUseTimestampInTicks -- controls timestamp format. MidiPlayer.getEventListFromSequence -- read from a specific sequence.
Source:
  ScriptingApiObjects.cpp:6250  getEventList() -> getEventListFromSequence(-1) -> HiseMidiSequence::getEventList()
