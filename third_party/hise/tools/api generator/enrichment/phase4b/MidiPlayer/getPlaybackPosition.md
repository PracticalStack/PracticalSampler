MidiPlayer::getPlaybackPosition() -> Double

Thread safety: SAFE -- Reads a cached double value.
Returns the current playback position as a normalised value (0.0-1.0) within the current loop range. Returns 0.0 if no sequence is loaded.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var pos = mp.getPlaybackPosition();
```
Pair with: MidiPlayer.setPlaybackPosition -- set position. MidiPlayer.getLastPlayedNotePosition -- position of last played note rather than transport position.
Source:
  ScriptingApiObjects.cpp:6250  getPlaybackPosition() -> MidiPlayer::getAttribute(CurrentPosition)
