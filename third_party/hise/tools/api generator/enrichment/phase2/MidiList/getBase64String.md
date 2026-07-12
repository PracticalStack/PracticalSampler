## getBase64String

**Examples:**

```javascript:save-midi-mapping-to-preset
// Title: Save a custom MIDI mapping to a user preset
// Context: A plugin allows users to remap which MIDI note triggers each
// sound. The mapping is stored in a MidiList and serialized to Base64
// for inclusion in user preset data.

const var noteToChannel = Engine.createMidiList();

// Simulate a user-configured mapping
noteToChannel.setValue(36, 0);
noteToChannel.setValue(38, 1);
noteToChannel.setValue(42, 2);

// Save the mapping as part of custom preset data
inline function getPresetData()
{
    local data = {};
    data.MidiMapping = noteToChannel.getBase64String();
    data.Version = 2;
    return data;
}

const var preset = getPresetData();
Console.print(isDefined(preset.MidiMapping));
Console.print(typeof preset.MidiMapping == "string");
```
```json:testMetadata:save-midi-mapping-to-preset
{
  "testable": true,
  "verifyScript": {
    "type": "log-output",
    "values": ["1", "1"]
  }
}
```

**Cross References:**
- `UserPresetHandler.setCustomAutomation` -- a common destination for MidiList-serialized data in custom preset systems.
