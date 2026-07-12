MidiPlayer::setFile(String fileName, Integer clearExistingSequences, Integer selectNewSequence) -> Integer

Thread safety: UNSAFE -- Loads from pool, allocates sequence objects, modifies sequence list.
Loads a MIDI file from the pool. Optionally clears existing sequences and selects the new one. Returns true if the file reference is valid (empty filename is a valid no-op). Use getMidiFileList() to get valid pool reference strings, or use {PROJECT_FOLDER} syntax.
Required setup:
```
const var mp = Synth.getMidiPlayer("MidiPlayer1");
mp.setFile("{PROJECT_FOLDER}myfile.mid", true, true);
```
Dispatch/mechanics: If clearExistingSequences, clears all with dontSendNotification. Loads via pool reference (FileHandlerBase::MidiFiles). If selectNewSequence, sets CurrentSequence attribute to the last loaded. Empty filename with selectNewSequence triggers a sequence update notification.
Pair with: MidiPlayer.getMidiFileList -- get valid pool references. MidiPlayer.clearAllSequences -- explicit clearing. MidiPlayer.create -- alternative to create empty sequences.
Source:
  ScriptingApiObjects.cpp:6250  setFile() -> pool load -> MidiPlayer::addSequence() -> setAttribute(CurrentSequence)
