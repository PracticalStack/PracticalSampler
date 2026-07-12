MidiPlayer::getPlayState() -> Integer

Thread safety: SAFE
Returns the current transport state: 0 = Stop, 1 = Play, 2 = Record.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var state = mp.getPlayState();
```
Pair with: MidiPlayer.play, MidiPlayer.stop, MidiPlayer.record -- control transport. MidiPlayer.setPlaybackCallback -- get notified on state changes.
Source:
  ScriptingApiObjects.cpp:6250  getPlayState() -> (int)MidiPlayer::getPlayState()
