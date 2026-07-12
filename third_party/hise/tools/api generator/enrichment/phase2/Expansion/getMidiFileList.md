## getMidiFileList

**Examples:**

```javascript:expansion-midi-file-list
// Title: Aggregating MIDI files from all expansions
// Context: Build a combined MIDI file list from the project root
// and all expansions for a MIDI player file selector.

const var Player = Synth.getMidiPlayer("MIDI Player1");
const var MidiSelector = Content.getComponent("MidiSelector");

// Start with the root project's MIDI files
const var allFiles = Player.getMidiFileList();
allFiles.insert(0, "None");

const var expHandler = Engine.createExpansionHandler();

// Append MIDI files from each expansion
for (e in expHandler.getExpansionList())
{
    for (f in e.getMidiFileList())
        allFiles.push(f);
}

MidiSelector.set("items", allFiles.join("\n"));

inline function onMidiSelectorControl(component, value)
{
    local ref = component.getItemText();

    if (ref == "None")
        ref = "";

    Player.setFile(ref, true, true);
};

MidiSelector.setControlCallback(onMidiSelectorControl);
```

```json:testMetadata:expansion-midi-file-list
{
  "testable": false,
  "skipReason": "Requires installed expansion packs with MIDI files"
}
```
