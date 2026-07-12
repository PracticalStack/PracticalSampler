## setAutomationDataFromObject

**Examples:**

```javascript:reroute-automation-layers
// Title: Re-route automation connections between parameter layers
// Context: When switching parameter layers (e.g., from individual layers
// to a linked "all" mode), existing CC assignments need their Attribute
// field updated to point to the new target parameter.

const var mah = Engine.createMidiAutomationHandler();

inline function transferConnections(oldSuffix, newSuffix)
{
    local data = mah.getAutomationDataObject();
    local changed = false;

    for (entry in data)
    {
        if (entry.Attribute.contains(oldSuffix))
        {
            entry.Attribute = entry.Attribute.replace(oldSuffix, newSuffix);
            changed = true;
        }
    }

    // Only write back if something actually changed
    if (changed)
        mah.setAutomationDataFromObject(data);
}

// Transfer connections from layer "A" to linked mode "All"
transferConnections(" A1", " All1");

// Verify that the handler is still functional after the operation
var verifyData = mah.getAutomationDataObject();
```
```json:testMetadata:reroute-automation-layers
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(verifyData)", "value": true},
    {"type": "REPL", "expression": "verifyData.length", "value": 0}
  ]
}
```

```javascript:undoable-cc-removal
// Title: Undoable CC assignment removal
// Context: A context menu action removes a CC assignment for a specific
// controller number. The operation is wrapped in an undo action so the
// user can revert it.

const var mah = Engine.createMidiAutomationHandler();

inline function removeAutomationWithUndo(ccNumber)
{
    local data = mah.getAutomationDataObject();
    local oldData = data.clone();

    for (i = 0; i < data.length; i++)
    {
        if (data[i].Controller == ccNumber)
        {
            data.removeElement(i);
            break;
        }
    }

    Engine.performUndoAction({
        "oldValue": oldData,
        "newValue": data
    }, function(isUndo, undoObject)
    {
        mah.setAutomationDataFromObject(isUndo ? this.oldValue : this.newValue);
    });
}

removeAutomationWithUndo(21);
```
```json:testMetadata:undoable-cc-removal
{
  "testable": false,
  "skipReason": "Engine.performUndoAction callback requires user-initiated undo/redo to trigger; no programmatic trigger available during onInit"
}
```

```javascript:clear-all-automation
// Title: Clear all automation connections with user confirmation
// Context: A "Clear All" button removes every CC assignment after
// showing a confirmation dialog.

const var mah = Engine.createMidiAutomationHandler();

// Pass an empty array to remove all automation entries
Engine.showYesNoWindow("Clear All", "Remove all CC assignments?",
    function(ok)
    {
        if (ok)
            mah.setAutomationDataFromObject([]);
    });
```
```json:testMetadata:clear-all-automation
{
  "testable": false,
  "skipReason": "Engine.showYesNoWindow requires user interaction to dismiss the dialog; callback cannot be triggered programmatically during onInit"
}
```

**Pitfalls:**
- When modifying the automation data array and writing it back, always work on the returned snapshot directly or `.clone()` it first. The returned array is a detached copy, so modifying entries (e.g., changing `entry.Attribute`) is safe before calling `setAutomationDataFromObject()`. However, calling `getAutomationDataObject()` again after modifications but before `setAutomationDataFromObject()` returns the old state, not your pending changes.
