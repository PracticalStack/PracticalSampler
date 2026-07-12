## setTableCallback

**Examples:**

```javascript:routing-table-events-through
// Title: Routing table events through a Broadcaster for multi-listener dispatch
// Context: Instead of a single inline function, pass a Broadcaster as the
//          table callback. This lets multiple listeners react to table events
//          independently -- one handles loading, another updates favorites,
//          a third manages preview playback.

const var table = Content.addViewport("DataTable", 0, 0);
table.setTableMode({ "RowHeight": 30, "Sortable": true, "HeaderHeight": 30 });

table.setTableColumns([
    { "ID": "Favorite", "Type": "Button", "MinWidth": 32,
      "Toggle": true,   "Focus": false, "Text": "Fav" },
    { "ID": "Name",     "Label": "Name",    "MinWidth": 200 },
    { "ID": "FilePath", "Type": "Hidden",   "MinWidth": 1 }
]);

const var tableBroadcaster = Engine.createBroadcaster({
    "id": "tableBroadcaster",
    "args": ["event"]
});

// Listener 1: Handle item selection and loading
tableBroadcaster.addListener("", "load item", function(event)
{
    if (event.Type == "DoubleClick" || event.Type == "ReturnKey")
        Console.print("Load: " + event.value.FilePath);
});

// Listener 2: Handle favorite toggle (needs original row index for sorted tables)
tableBroadcaster.addListener(table, "toggle favorite", function(event)
{
    if (event.columnID == "Favorite")
    {
        local originalIndex = this.getOriginalRowIndex(event.rowIndex);
        Console.print("Toggle favorite at original index: " + originalIndex);
    }
});

// Pass the Broadcaster directly as the callback
table.setTableCallback(tableBroadcaster);

```
```json:testMetadata:routing-table-events-through
{
  "testable": false
}
```

```javascript:modulation-matrix-callback-dispatching
// Title: Modulation matrix callback dispatching multiple event types
// Context: A table displaying modulation connections with Slider, ComboBox,
//          and Button cells. The callback switches on event.Type to route
//          each interaction to the appropriate handler.

const var modTable = Content.addViewport("ModTable", 0, 0);
modTable.setTableMode({ "RowHeight": 32, "HeaderHeight": 32 });

modTable.setTableColumns([
    { "ID": "Source",    "Type": "Text",     "MinWidth": 150 },
    { "ID": "Target",    "Type": "Text",     "MinWidth": 170 },
    { "ID": "Intensity", "Type": "Slider",   "MinWidth": 110 },
    { "ID": "Mode",      "Type": "ComboBox", "MinWidth": 80,
      "ValueMode": "Text", "Text": "Default" },
    { "ID": "Delete",    "Type": "Button",   "MinWidth": 32,
      "Toggle": false,   "Text": "Delete" }
]);

inline function onModTableEvent(event)
{
    if (event.Type == "Slider")
    {
        // event.value is the slider's current value
        Console.print("Intensity changed: " + event.value);
        return;
    }

    if (event.Type == "ComboBox" && event.columnID == "Mode")
    {
        // event.value is the selected text (when ValueMode is "Text")
        Console.print("Mode changed to: " + event.value);
        return;
    }

    if (event.Type == "Button" && event.columnID == "Delete")
    {
        // Momentary button: event.value toggles, act on release (false)
        if (!event.value)
            Console.print("Delete row " + event.rowIndex);

        return;
    }

    if (event.Type == "Click" || event.Type == "Selection")
    {
        // event.value is the full row data object
        Console.print("Selected: " + event.value.Source + " -> " + event.value.Target);
    }
};

modTable.setTableCallback(onModTableEvent);

```
```json:testMetadata:modulation-matrix-callback-dispatching
{
  "testable": false
}
```

**Pitfalls:**
- The `event.value` property changes meaning depending on `event.Type`: for Click/DoubleClick/Selection/ReturnKey/DeleteRow it is the full row data object; for Slider it is the numeric slider value; for Button it is the toggle state (boolean); for ComboBox it is the selected item (format depends on ValueMode). Always check `event.Type` before accessing `event.value`.
- A Broadcaster can be passed directly instead of an inline function. When the table fires, the Broadcaster receives the event object as its message, enabling fan-out to multiple listeners without coupling the table to specific handlers.
