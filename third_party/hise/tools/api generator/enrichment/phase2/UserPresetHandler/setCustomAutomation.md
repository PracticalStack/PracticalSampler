## setCustomAutomation

**Examples:**

```javascript:programmatic-automation-data
// Title: Programmatic automation data generation for a multi-channel instrument
// Context: When a plugin has many channels with identical parameter sets, building
// the automation data array programmatically avoids hundreds of lines of manual JSON.
// Each channel gets its own set of automation slots with processor connections, and
// "ABC" (linked) slots use MetaConnections to route to per-layer slots.

const var NUM_CHANNELS = 4;
const var LAYER_NAMES = ["A", "B", "C"];

const var uph = Engine.createUserPresetHandler();

inline function onLoad(data) {};
inline function onSave(name) { return {}; };
uph.setUseCustomUserPresetModel(onLoad, onSave, false);

// Build the automation data programmatically
var automationData = [];

for (i = 0; i < NUM_CHANNELS; i++)
{
    // Per-layer slots (ProcessorConnection) - must come FIRST for MetaConnections
    for (layer in LAYER_NAMES)
    {
        automationData.push({
            "ID": "Gain " + layer + (i + 1),
            "min": -100.0,
            "max": 0.0,
            "mode": "Decibel",
            "defaultValue": -12.0,
            "allowHostAutomation": false,
            "connections": [
                { "processorId": "Player " + layer + (i + 1), "parameterId": "Gain" }
            ]
        });
    }

    // Linked ABC slot (MetaConnection) - references per-layer slots above
    automationData.push({
        "ID": "Gain ABC" + (i + 1),
        "min": -100.0,
        "max": 0.0,
        "mode": "Decibel",
        "defaultValue": -12.0,
        "allowHostAutomation": true,
        "connections": [
            { "automationId": "Gain A" + (i + 1) },
            { "automationId": "Gain B" + (i + 1) },
            { "automationId": "Gain C" + (i + 1) }
        ]
    });

    // Mixer XY position (CableConnection)
    automationData.push({
        "ID": "Mixer " + (i + 1) + " X",
        "min": 0.0,
        "max": 1.0,
        "mode": "NormalizedPercentage",
        "defaultValue": 0.5,
        "allowHostAutomation": true,
        "connections": [
            { "processorId": "Mixer " + (i + 1), "parameterId": "X" }
        ]
    });
}

uph.setCustomAutomation(automationData);
```
```json:testMetadata:programmatic-automation-data
{
  "testable": false,
  "skipReason": "Requires Player and Mixer modules in the module tree matching the generated processor IDs."
}
```

**Pitfalls:**
- MetaConnection targets (referenced via `automationId`) must appear earlier in the automation array than the slot that references them. In a programmatic generation loop, push per-layer slots before the linked "ABC" slot.
- When generating large automation arrays programmatically, set `allowHostAutomation: false` on internal per-layer slots and `allowHostAutomation: true` only on the linked/master slots. This prevents the DAW from exposing hundreds of redundant parameters.
