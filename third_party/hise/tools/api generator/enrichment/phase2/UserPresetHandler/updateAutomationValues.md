## updateAutomationValues

**Examples:**

```javascript:two-input-modes
// Title: Two input modes - batch value update and sync-from-processors
// Context: updateAutomationValues has two distinct modes depending on whether
// the first argument is an Array or an Integer.

const var uph = Engine.createUserPresetHandler();

// MODE 1: Array of objects - set specific automation values
// Used during custom preset load to restore automation state
uph.updateAutomationValues([
    {"id": "Volume 1", "value": -6.0},
    {"id": "Pan 1", "value": 0.5},
    {"id": "FilterFreq 1", "value": 1000.0}
], SyncNotification, false);

// MODE 2: Integer - refresh all slots from their processor connections
// The integer is the preferred connection index within each slot's
// connection list. Pass 0 to read from the first (primary) connection.
// Used after programmatic module changes to sync automation values back.
uph.updateAutomationValues(0, SyncNotification, false);
```
```json:testMetadata:two-input-modes
{
  "testable": false,
  "skipReason": "Requires setCustomAutomation with registered slots and connected processors."
}
```

```javascript:reset-to-defaults
// Title: Resetting all automation values to defaults
// Context: When no preset data is available (e.g., loading a legacy format
// that predates the automation system), build a default-values array from
// the automation data definitions and apply it.

const var uph = Engine.createUserPresetHandler();

// The same array passed to setCustomAutomation - keep a reference at init
const var automationData = [
    {"ID": "Volume", "min": -100.0, "max": 0.0, "defaultValue": -12.0, "connections": []},
    {"ID": "Pan", "min": -1.0, "max": 1.0, "defaultValue": 0.0, "connections": []}
];

var defaultValues = [];

for (entry in automationData)
    defaultValues.push({"id": entry.ID, "value": entry.defaultValue});

uph.updateAutomationValues(defaultValues, SyncNotification, false);
```
```json:testMetadata:reset-to-defaults
{
  "testable": false,
  "skipReason": "Requires automationData array and setCustomAutomation setup."
}
```
