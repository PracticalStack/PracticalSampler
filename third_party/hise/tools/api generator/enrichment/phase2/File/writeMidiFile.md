## writeMidiFile

**Examples:**

```javascript:export-midi-unique-name
// Title: Export a MIDI pattern to a uniquely-named .mid file
// Context: Exporting sequencer pattern data as a standard MIDI file

const var tempDir = FileSystem.getFolder(FileSystem.Temp);

inline function exportMidiPattern(eventList, metadata)
{
    // Generate a unique filename to avoid overwriting
    local targetFile = tempDir.getChildFile("Pattern.mid").getNonExistentSibling();

    // Clean up any stale file at this path
    targetFile.deleteFileOrDirectory();

    local ok = targetFile.writeMidiFile(eventList, metadata);

    if (ok)
        Console.print("MIDI exported: " + targetFile.toString(3));

    return targetFile;
}

// Create a single-note event and export
var events = [];
var noteOn = Engine.createMessageHolder();
noteOn.setType(noteOn.NoteOn);
noteOn.setNoteNumber(60);
noteOn.setVelocity(100);
noteOn.setTimestamp(0);
events.push(noteOn);

exportMidiPattern(events, {"Tempo": 120.0, "NumBars": 1});
```
```json:testMetadata:export-midi-unique-name
{
  "testable": false,
  "skipReason": "Writes to temp filesystem and requires cleanup"
}
```
