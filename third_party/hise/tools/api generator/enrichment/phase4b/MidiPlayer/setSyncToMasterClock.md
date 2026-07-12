MidiPlayer::setSyncToMasterClock(Integer shouldSyncToMasterClock) -> undefined

Thread safety: UNSAFE -- Modifies player state and validates master clock availability.
Enables or disables synchronisation to the master clock (host transport or internal clock). When enabled, play() and stop() become no-ops -- transport is driven by the master clock.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setSyncToMasterClock(true);
```
Dispatch/mechanics: Sets syncToMasterClock flag on MidiPlayer core. When true, play()/stop() return false without acting. Transport driven by onGridChange()/onTransportChange() from TempoListener. record() from stop defers via recordOnNextPlaybackStart flag.
Pair with: MidiPlayer.play, MidiPlayer.stop -- become no-ops when synced. MidiPlayer.record -- has special deferred handling when synced.
Anti-patterns: Must enable the master clock grid before calling with true, otherwise a script error is thrown. When synced, play() and stop() silently return false.
Source:
  ScriptingApiObjects.cpp:6250  setSyncToMasterClock() -> sets MidiPlayer::syncToMasterClock flag
