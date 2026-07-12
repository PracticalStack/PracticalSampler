## setValue

**Examples:**

```javascript:broadcaster-sync-selection
// Title: Sync combo box selection from a Broadcaster
// Context: A Broadcaster drives the combo box value from an external source
// (e.g., a zoom level changed elsewhere). The combo box reflects the current
// state without triggering its own callback.

const var ZOOM_LEVELS = [0.75, 1.0, 1.25, 1.5, 2.0];

const var cbZoom = Content.addComboBox("ZoomSelector", 0, 0);
cbZoom.set("items", "75%\n100%\n125%\n150%\n200%");
cbZoom.set("saveInPreset", false);

const var zoomBroadcaster = Engine.createBroadcaster({
    "id": "zoomSync",
    "args": ["value"]
});

// When the broadcaster fires, update the combo box to match
zoomBroadcaster.addListener(cbZoom, "update zoom selector", function(value)
{
    // Find which index matches the zoom value and select it
    local idx = ZOOM_LEVELS.indexOf(value) + 1;
    this.setValue(idx);
});

// --- test-only ---
zoomBroadcaster.sendSyncMessage([1.25]);
// --- end test-only ---
```
```json:testMetadata:broadcaster-sync-selection
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cbZoom.getValue()", "value": 3},
    {"type": "REPL", "expression": "cbZoom.getItemText()", "value": "125%"}
  ]
}
```

```javascript:clamp-after-rebuild
// Title: Clamp selection after rebuilding items
// Context: When a dependent combo box has its items rebuilt, the previous selection
// index may exceed the new item count. Clamp to the new max before calling changed().

const var cbOption = Content.addComboBox("Option", 0, 0);
cbOption.set("items", "A\nB\nC\nD\nE");
cbOption.set("saveInPreset", false);
cbOption.setValue(4); // Select "D"

// Later, the items are rebuilt with fewer entries
cbOption.set("items", "X\nY\nZ");

// Previous value (4) is now out of range -- clamp to new max (3)
cbOption.setValue(Math.min(parseInt(cbOption.getValue()), cbOption.get("max")));
```
```json:testMetadata:clamp-after-rebuild
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "cbOption.getValue()", "value": 3},
    {"type": "REPL", "expression": "cbOption.getItemText()", "value": "Z"}
  ]
}
```

**Pitfalls:**
- Do not pass a string value. `setValue("Sine")` reports a script error. Combo boxes use 1-based integer indices, not item text.
