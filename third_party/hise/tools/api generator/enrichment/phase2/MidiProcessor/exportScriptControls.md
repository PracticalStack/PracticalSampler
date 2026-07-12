## exportScriptControls

**Examples:**

```javascript:factory-preset-save-load
// Title: Factory preset system -- saving script processor UI state
// Context: A custom preset system saves the UI control values of each script
// processor separately from modulators and effects. exportScriptControls()
// captures only knob/slider/button values without the script code itself,
// and restoreScriptControls() restores them without triggering recompilation.

const var midiProcessorIds = Synth.getIdList("MidiProcessor");
const var presetData = {"midiProcessors": {}};

inline function savePresetData()
{
    for (id in midiProcessorIds)
    {
        local mp = Synth.getMidiProcessor(id);

        // exportScriptControls only works on script processors --
        // built-in modules like Transposer or Arpeggiator will throw an error.
        // Use exportState() for those instead.
        presetData.midiProcessors[id] = mp.exportScriptControls();
    }
}

inline function loadPresetData()
{
    for (id in midiProcessorIds)
    {
        local mp = Synth.getMidiProcessor(id);

        if (typeof presetData.midiProcessors[id] == void)
            continue;

        // Restores UI control values without recompiling the script
        mp.restoreScriptControls(presetData.midiProcessors[id]);
    }
}
```
```json:testMetadata:factory-preset-save-load
{
  "testable": false,
  "skipReason": "Requires script processors in the module tree; utility functions defined but not invoked"
}
```

**Pitfalls:**
- When iterating all MIDI processors via `Synth.getIdList("MidiProcessor")`, the list includes both script processors and built-in modules. Guard calls to `exportScriptControls()` with a try/catch or filter the list to known script processor IDs, since calling it on a built-in module throws a runtime error.
