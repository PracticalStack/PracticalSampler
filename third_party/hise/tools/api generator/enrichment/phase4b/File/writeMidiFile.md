File::writeMidiFile(var eventList, var metadataObject) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (MIDI encoding and writing). Allocates HiseMidiSequence.
Writes an array of MessageHolder objects as a standard MIDI file (.mid).
Returns true on success, false if eventList is not an Array.

Metadata object: same TimeSignature format as File.loadMidiMetadata (all
  properties optional). NumBars=0 triggers auto-calculation from last event.

Dispatch/mechanics:
  Creates HiseMidiSequence, sets TimeSignature from metadata
  -> MidiPlayer::EditAction::writeArrayToSequence (casts each element to
     ScriptingMessageHolder, skips non-MessageHolder elements silently)
  -> writes to temp file, then moves to target path
  Uses HISE internal resolution of 960 ticks per quarter note.

Pair with:
  loadAsMidiFile -- to read MIDI data from disk
  Engine.createMessageHolder -- to create event objects for the array

Anti-patterns:
  - [BUG] Non-Array eventList silently returns false without error.
  - [BUG] Non-MessageHolder elements are silently skipped. An array of plain
    objects produces an empty MIDI file with no error.
  - NumBars auto-calculation assumes tick-based timestamps. Sample-based timestamps
    produce incorrect bar counts.

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeMidiFile()
    -> HiseMidiSequence creation -> writeArrayToSequence
    -> writes to temp file, moves to target
