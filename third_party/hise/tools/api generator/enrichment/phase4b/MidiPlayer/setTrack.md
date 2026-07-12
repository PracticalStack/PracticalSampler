MidiPlayer::setTrack(Integer trackIndex) -> undefined

Thread safety: UNSAFE -- Calls setAttribute with sendNotificationAsync.
Sets the active track within the current sequence. Uses one-based indexing. Affects which track is read by getEventList() and written to by flushMessageList().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setTrack(1);
```
Pair with: MidiPlayer.getNumTracks -- get track count to validate index.
Anti-patterns: Index must be >= 1. Sequence and track indices are one-based; index 0 triggers a script error.
Source:
  ScriptingApiObjects.cpp:6250  setTrack() -> MidiPlayer::setAttribute(CurrentTrack, sendNotificationAsync)
