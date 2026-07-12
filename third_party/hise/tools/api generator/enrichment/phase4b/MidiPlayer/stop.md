MidiPlayer::stop(Integer timestamp) -> Integer

Thread safety: WARNING -- Delegates to MidiPlayer::stop() which modifies transport state and calls sendPlaybackChangeMessage(). When syncToMasterClock is true, this is a no-op (returns false).
Stops playback or recording. Pass 0 for immediate stop, or a timestamp for sample-accurate stopping. Returns true if stopped, false if synced to master clock. If recording, flushes recorded events to the sequence. Sends note-off messages for sounding notes and handles sustain pedal cleanup.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.stop(0);
```
Dispatch/mechanics: If syncToMasterClock, clears recordOnNextPlaybackStart and returns false. Otherwise calls stopInternal() which finishes recording if active, adds note-offs for pending notes, handles sustain cleanup, resets playback state, sets playState to Stop and currentPosition to -1, then calls sendPlaybackChangeMessage().
Pair with: MidiPlayer.play -- start playback. MidiPlayer.record -- start recording. MidiPlayer.setSyncToMasterClock -- controls whether stop() is a no-op.
Source:
  ScriptingApiObjects.cpp:6250  stop() -> MidiPlayer::stop() -> stopInternal() -> finishRecording() -> sendPlaybackChangeMessage()
