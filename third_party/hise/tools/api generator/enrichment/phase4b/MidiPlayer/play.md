MidiPlayer::play(Integer timestamp) -> Integer

Thread safety: WARNING -- Delegates to MidiPlayer::play() which modifies transport state and calls sendPlaybackChangeMessage(). Safe for audio thread calling but triggers listener notifications. When syncToMasterClock is true, this is a no-op (returns false).
Starts playback. Pass 0 for immediate start, or a sample-accurate timestamp to delay start within the current buffer. Returns true if playback started, false if synced to master clock.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.play(0);
```
Dispatch/mechanics: Checks syncToMasterClock -- if true and not recording, returns false. Otherwise calls startInternal() which resets position to 0, sets playState to Play, and calls sendPlaybackChangeMessage() to notify all PlaybackListeners.
Pair with: MidiPlayer.stop -- stop playback. MidiPlayer.record -- start recording. MidiPlayer.getPlayState -- query current state. MidiPlayer.setSyncToMasterClock -- controls whether play() is a no-op.
Source:
  ScriptingApiObjects.cpp:6250  play() -> MidiPlayer::play() -> startInternal() -> sendPlaybackChangeMessage()
