## getIdList

**Examples:**

```javascript:populate-combobox-from-id-list
// Title: Dynamically populate a ComboBox with available voice start modulators
// Context: A detune/unison script discovers which Script Voice Start Modulators
// exist in the module tree and lets the user assign them to pitch, pan, and gain
// roles via ComboBox dropdowns. This avoids hard-coding processor IDs.

const var pitchSelector = Content.addComboBox("PitchModSelector", 100, 10);
const var gainSelector = Content.addComboBox("GainModSelector", 250, 10);

// getIdList takes a processor TYPE name, not a processor ID
const var voiceStartMods = Synth.getIdList("Script Voice Start Modulator");

pitchSelector.set("items", "None");
gainSelector.set("items", "None");

for (id in voiceStartMods)
{
    pitchSelector.addItem(id);
    gainSelector.addItem(id);
}

reg pitchDetuner;
reg gainDetuner;

inline function onPitchSelectorControl(component, value)
{
    local name = component.getItemText();

    if (name.length > 0)
        pitchDetuner = Synth.getModulator(name);
}

pitchSelector.setControlCallback(onPitchSelectorControl);
```
```json:testMetadata:populate-combobox-from-id-list
{
  "testable": false,
  "skipReason": "Requires a module tree with Script Voice Start Modulator processors present for getIdList to return results"
}
```

**Pitfalls:**
- The `type` parameter is the processor's **type name** (e.g., "Script Voice Start Modulator", "LFO Modulator", "Simple Gain"), not a user-assigned processor ID. Passing a processor ID like "MyLFO" returns an empty array with no error - a common and hard-to-diagnose mistake.
