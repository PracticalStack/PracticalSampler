MidiPlayer::getMidiFileList() -> Array

Thread safety: UNSAFE -- Iterates the MIDI file pool and constructs String objects.
Returns an array of string references for all MIDI files embedded in the plugin's MIDI file pool. These reference strings can be passed to setFile() to load a specific MIDI file.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
var files = mp.getMidiFileList();
```
Pair with: MidiPlayer.setFile -- load a file using a reference from this list.
Source:
  ScriptingApiObjects.cpp:6250  getMidiFileList() -> FileHandlerBase::MidiFiles pool iteration
