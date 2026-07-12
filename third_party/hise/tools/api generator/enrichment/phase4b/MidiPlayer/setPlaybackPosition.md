MidiPlayer::setPlaybackPosition(Double newPosition) -> undefined

Thread safety: UNSAFE -- Calls setAttribute with sendNotificationAsync, which involves attribute change notification.
Sets the playback position within the current loop. Value is clamped to 0.0-1.0. Does nothing if no sequence is loaded.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setPlaybackPosition(0.5);
```
Pair with: MidiPlayer.getPlaybackPosition -- read back the current position.
Source:
  ScriptingApiObjects.cpp:6250  setPlaybackPosition() -> MidiPlayer::setAttribute(CurrentPosition, sendNotificationAsync)
