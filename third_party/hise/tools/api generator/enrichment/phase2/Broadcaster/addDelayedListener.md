## addDelayedListener

**Examples:**

```javascript:debounce-filter-changes
// Title: Debouncing preset browser filter changes
// Context: When a preset browser has multiple filter controls (search field, tag list,
// favorites toggle), each change should rebuild the table, but rapid consecutive
// changes should only trigger one rebuild. A 30ms delayed listener debounces this.

// --- setup ---
const var SearchField = Content.addKnob("SearchField", 0, 0);
SearchField.set("saveInPreset", false);
const var TagList = Content.addKnob("TagList", 150, 0);
TagList.set("saveInPreset", false);
const var FavoriteToggle = Content.addButton("FavoriteToggle", 300, 0);
FavoriteToggle.set("saveInPreset", false);
// --- end setup ---

const var filterBc = Engine.createBroadcaster({
    "id": "PresetFilter",
    "args": ["component", "value"]
});

filterBc.attachToComponentValue(
    [SearchField, TagList, FavoriteToggle],
    "filterInputs"
);

var rebuildCount = 0;

// The delayed listener fires only after 30ms of inactivity.
// Rapid changes to search + tags + favorites produce a single rebuild.
filterBc.addDelayedListener(30, "browser", "rebuildTable",
    function(component, value)
{
    rebuildCount++;
});
```
```json:testMetadata:debounce-filter-changes
{
  "testable": false,
  "skipReason": "addDelayedListener timer callback causes HISE Debug crash during validation - suspected debug-only assertion in delayed timer dispatch"
}
```

```javascript:drag-bypass-reactivation
// Title: Re-enabling a broadcaster after user interaction (drag-bypass pattern)
// Context: After the user finishes dragging a knob, a 100ms delayed listener
// re-enables a parameter-monitoring broadcaster that was bypassed during the drag.

// --- setup ---
const var DragKnob1 = Content.addKnob("DragKnob1", 0, 0);
DragKnob1.set("saveInPreset", false);
const var DragKnob2 = Content.addKnob("DragKnob2", 150, 0);
DragKnob2.set("saveInPreset", false);
// --- end setup ---

const var mouseGuard = Engine.createBroadcaster({
    "id": "MouseGuard",
    "args": ["component", "event"]
});

const var paramMonitor = Engine.createBroadcaster({
    "id": "ParamMonitor",
    "args": ["processorId", "parameterId", "value"]
});

mouseGuard.attachToComponentMouseEvents(
    [DragKnob1, DragKnob2],
    "Clicks Only",
    "knobClicks"
);

// Pass the broadcaster to protect as `this` via the object parameter
mouseGuard.addDelayedListener(100, paramMonitor, "reactivate",
    function(component, event)
{
    if (isDefined(event.mouseUp))
        this.setBypassed(false, false, SyncNotification);
});
```
```json:testMetadata:drag-bypass-reactivation
{
  "testable": false,
  "skipReason": "Mouse events require physical user interaction that cannot be triggered programmatically from script"
}
```

The delayed listener always receives the most recent values at the time the timer fires, not the values from the send that started the timer. This makes it ideal for debouncing where only the final state matters.
