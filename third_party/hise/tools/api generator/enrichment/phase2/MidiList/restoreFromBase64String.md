## restoreFromBase64String

**Examples:**

```javascript:restore-midi-mapping-from-preset
// Title: Restore a custom MIDI mapping from user preset data
// Context: When loading a user preset, the serialized MidiList is
// decoded and applied to restore the user's custom MIDI mapping.

const var noteToChannel = Engine.createMidiList();

// Simulate saving a mapping
noteToChannel.setValue(36, 0);
noteToChannel.setValue(38, 1);
noteToChannel.setValue(42, 2);
const var saved = noteToChannel.getBase64String();

// Simulate loading from preset data
noteToChannel.clear();

inline function loadPresetData(data)
{
    if (isDefined(data.MidiMapping))
        noteToChannel.restoreFromBase64String(data.MidiMapping);
}

loadPresetData({"MidiMapping": saved});

Console.print(noteToChannel.getValue(36));
Console.print(noteToChannel.getValue(38));
Console.print(noteToChannel.getValue(42));
Console.print(noteToChannel.getValue(60));
```
```json:testMetadata:restore-midi-mapping-from-preset
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": ["0", "1", "2", "-1"]
  }
}
```

**Pitfalls:**
- Always guard `restoreFromBase64String` with `isDefined()` or a version check. Older presets may not contain the serialized MidiList field, and passing an empty or undefined string will produce unexpected results.
