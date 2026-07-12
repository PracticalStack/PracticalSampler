MidiPlayer::record(Integer timestamp) -> Integer

Thread safety: WARNING -- Delegates to MidiPlayer::record() which modifies transport state. When syncToMasterClock is true and stopped, defers via recordOnNextPlaybackStart flag (returns false).
Starts recording incoming MIDI events into the current sequence. Pass 0 for immediate start. Returns true if recording started, false if deferred (synced to master clock and stopped). Uses overdub mode by default, merging new notes with existing data.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.record(0);
```
Dispatch/mechanics: If syncToMasterClock and stopped, sets recordOnNextPlaybackStart flag and returns false. Otherwise calls recordInternal() which starts overdub updater, sets playState to Record, calls sendPlaybackChangeMessage(), and prepares for recording if idle.
Pair with: MidiPlayer.play -- start playback. MidiPlayer.stop -- stop and flush recorded events. MidiPlayer.setRecordEventCallback -- filter/modify events during recording.
Source:
  ScriptingApiObjects.cpp:6250  record() -> MidiPlayer::record() -> recordInternal() -> prepareForRecording() -> sendPlaybackChangeMessage()
