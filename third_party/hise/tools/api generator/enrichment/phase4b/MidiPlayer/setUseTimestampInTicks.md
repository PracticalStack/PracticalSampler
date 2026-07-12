MidiPlayer::setUseTimestampInTicks(Integer shouldUseTicksAsTimestamps) -> undefined

Thread safety: SAFE
Switches timestamp format for event list operations between samples (default) and MIDI ticks (960 per quarter). Tick mode is recommended for musical editing (quantization, grid alignment) since tick values are tempo-independent. Affects getEventList(), getEventListFromSequence(), flushMessageList(), and flushMessageListToSequence().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setUseTimestampInTicks(true);
```
Pair with: MidiPlayer.getTicksPerQuarter -- get the tick resolution (960). MidiPlayer.getEventList, MidiPlayer.flushMessageList -- affected by this setting.
Source:
  ScriptingApiObjects.cpp:6250  setUseTimestampInTicks() -> sets useTicks flag on ScriptedMidiPlayer
