MidiPlayer::setPlaybackCallback(Function playbackCallback, Number synchronous) -> undefined

Thread safety: UNSAFE -- Creates a PlaybackUpdater object, registers as PlaybackListener.
Registers a callback that fires on transport state changes (play, stop, record). The synchronous parameter controls threading: 0 = async (UI thread via deferred timer), non-zero = synchronous (audio thread, callback must be an inline function).
Callback signature: f(int timestamp, int playState)
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
inline function onPlaybackChange(timestamp, playState) { /* ... */ }
mp.setPlaybackCallback(onPlaybackChange, 0);
```
Dispatch/mechanics: Creates a PlaybackUpdater that registers as MidiPlayer::PlaybackListener. When sync, playbackChanged() calls callSync() directly on the audio thread. When async, sets dirty flag and defers to timerCallback() on UI thread.
Pair with: MidiPlayer.getPlayState -- query state without callback. MidiPlayer.play, MidiPlayer.stop, MidiPlayer.record -- trigger state changes.
Anti-patterns: When using synchronous mode, the callback runs on the audio thread. In the HISE IDE, a realtime safety check validates the callback. In exported plugins, only isRealtimeSafe() is checked.
Source:
  ScriptingApiObjects.cpp:9744  PlaybackUpdater() -> MidiPlayer::addPlaybackListener() -> playbackChanged() -> callSync()/dirty flag
