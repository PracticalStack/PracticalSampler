MidiPlayer::getLastPlayedNotePosition() -> Double

Thread safety: SAFE -- Reads a cached value from the sequence.
Returns the normalised position (0.0-1.0) of the last note played during playback. Returns -1 if the player is stopped.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var pos = mp.getLastPlayedNotePosition();
```
Pair with: MidiPlayer.getPlaybackPosition -- gets the current transport position rather than last note position.
Source:
  ScriptingApiObjects.cpp:6250  getLastPlayedNotePosition() -> HiseMidiSequence::lastPlayedIndex (cached)
