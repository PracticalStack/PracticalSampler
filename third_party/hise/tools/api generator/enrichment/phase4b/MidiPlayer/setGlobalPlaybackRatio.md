MidiPlayer::setGlobalPlaybackRatio(Double globalRatio) -> undefined

Thread safety: SAFE -- Sets an atomic value on MainController.
Sets a global playback speed multiplier affecting ALL MidiPlayer instances. Effective speed = playerSpeed * globalRatio.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setGlobalPlaybackRatio(0.5);
```
Anti-patterns: This affects ALL MidiPlayer instances globally, not just the one you call it on. The setting lives on the MainController.
Source:
  ScriptingApiObjects.cpp:6250  setGlobalPlaybackRatio() -> MainController::setGlobalMidiPlaybackSpeed()
