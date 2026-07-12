MidiPlayer::reset() -> undefined

Thread safety: UNSAFE -- Resets sequence data, involves pool operations.
Resets the current sequence to the state it was in when last loaded from a MIDI file. Discards any edits made via flushMessageList() or recording. Does nothing if no sequence is loaded.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.reset();
```
Pair with: MidiPlayer.undo -- undo individual edits rather than full reset. MidiPlayer.setFile -- reload from file.
Source:
  ScriptingApiObjects.cpp:6250  reset() -> MidiPlayer core reset (pool reload)
