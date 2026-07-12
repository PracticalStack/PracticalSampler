MidiPlayer::saveAsMidiFile(String file, Integer trackIndex) -> Integer

Thread safety: UNSAFE -- File I/O operation, writes to disk and reloads pool.
Saves the current sequence to a MIDI file at the given path, writing data to the specified track index in the output file. Returns true on success. After saving, the MIDI file pool is reloaded so the file is immediately available via getMidiFileList().
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.saveAsMidiFile("{PROJECT_FOLDER}output.mid", 0);
```
Dispatch/mechanics: Delegates to MidiPlayer::saveAsMidiFile() which writes to pool, adds time signature and end-of-track meta events, creates/expands the MIDI file with proper track indexing, then reloads the pool.
Pair with: MidiPlayer.setFile -- load a saved file back. MidiPlayer.getMidiFileList -- verify file appears in pool after save.
Anti-patterns: Empty string for file path triggers a script error.
Source:
  ScriptingApiObjects.cpp:6250  saveAsMidiFile() -> MidiPlayer::saveAsMidiFile() -> pool write -> pool reload
