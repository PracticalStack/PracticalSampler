MidiPlayer::getNumTracks() -> Integer

Thread safety: SAFE
Returns the number of tracks in the current sequence. Returns 0 if no sequence is loaded or no player reference exists.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var num = mp.getNumTracks();
```
Pair with: MidiPlayer.setTrack -- select a track by index.
Source:
  ScriptingApiObjects.cpp:6250  getNumTracks() -> HiseMidiSequence::sequences.size()
