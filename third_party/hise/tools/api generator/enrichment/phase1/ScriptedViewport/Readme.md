# ScriptedViewport -- Class Analysis

## Brief
Multi-mode viewport component supporting scrollable viewport, selectable list, and interactive table modes.

## Purpose
ScriptedViewport is a versatile UI component that operates in three distinct modes: Viewport mode (a scrollable container for child components), List mode (a selectable list of text items), and Table mode (a fully interactive data table with sortable columns and editable cells). The mode is determined at init time -- calling `setTableMode()` activates Table mode, setting the `useList` property activates List mode, and the default is a plain scrollable Viewport. Table mode supports Text, Button, Slider, ComboBox, Image, and Hidden cell types, with a rich callback system for user interactions.

## Details

### Operating Modes

| Mode | Activation | JUCE Component | Value Type |
|------|-----------|----------------|------------|
| Viewport | Default (no setTableMode, useList=false) | Viewport with 4000x4000 dummy child | Arbitrary (set via setValue) |
| List | Set useList=true property | ListBox | Integer row index (0-based) |
| Table | Call setTableMode() in onInit | TableListBox | Integer or [column, row] array (if MultiColumnMode) |

### Table Mode Architecture

Table mode is powered by the ScriptTableListModel class. Setup requires a mandatory sequence: `setTableMode()` first (creates the model), then `setTableColumns()` (defines column structure), then `setTableRowData()` (populates rows). Both `setTableMode()` and `setTableColumns()` must be called in `onInit`. Row data can be updated at any time.

The table supports six cell types: Text (read-only display), Button (momentary or toggle), Slider (with configurable range and style), ComboBox (with ID/Index/Text value modes), Image, and Hidden. Interactive cells (Button, Slider, ComboBox) fire specialized callbacks with event type strings.

### Table Callback System

The callback registered via `setTableCallback()` receives a single JSON object argument with these properties: `Type` (event type string), `rowIndex` (int, -1 for background click), `columnID` (the ID from column metadata), and `value` (content depends on event type).

Event type strings: "Click", "DoubleClick", "Selection", "ReturnKey", "SpaceKey", "DeleteRow", "Slider", "Button", "ComboBox", "SetValue".

### Scroll Position Tracking

In all modes, the `viewPositionX` and `viewPositionY` properties (0.0-1.0 normalized) track and control the scroll position. These are bidirectionally synced with the underlying viewport via `positionBroadcaster`.

### Component-Specific Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| scrollBarThickness | Number | 16 | Scrollbar width in pixels (0-40) |
| autoHide | Toggle | true | Auto-hide scrollbars when not needed |
| useList | Toggle | false | Enable list mode (newline-separated items) |
| viewPositionX | Number | 0.0 | Normalized horizontal scroll position (0-1) |
| viewPositionY | Number | 0.0 | Normalized vertical scroll position (0-1) |
| items | String | "" | Newline-separated list of items (List mode) |
| fontName | String | "Arial" | Font family for text rendering |
| fontSize | Number | 13 | Font size in pixels (1-200) |
| fontStyle | String | "plain" | Font style (plain, bold, italic, etc.) |
| alignment | String | "centred" | Text alignment (centred, left, right, etc.) |

### Deactivated Base Properties

`macroControl`, `min`, `max` -- viewport does not use numeric ranges or macro connections.

### LAF Functions

Table/List mode: `drawTableRowBackground`, `drawTableCell`, `drawTableHeaderBackground`, `drawTableHeaderColumn`, `drawScrollbar`

## obtainedVia
`Content.addViewport(name, x, y)`

## minimalObjectToken
Viewport1

## Constants
(No constants registered.)

## Dynamic Constants
(None.)

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Viewport1.setTableMode({});` called outside onInit | `// In onInit:\nViewport1.setTableMode({});` | setTableMode, setTableColumns, and setTableCallback must be called in onInit only. |
| `Viewport1.setTableColumns([...]);` without calling setTableMode first | `Viewport1.setTableMode({});\nViewport1.setTableColumns([...]);` | setTableColumns requires setTableMode to be called first to create the internal table model. |
| Using `function` for setTableCallback | Using `inline function` for setTableCallback | Table callbacks must use inline function syntax for safety. |

## codeExample
```javascript
// Table mode: define columns and populate rows
const var Viewport1 = Content.addViewport("Viewport1", 0, 0);

Viewport1.setTableMode({ "RowHeight": 30, "HeaderHeight": 28 });

Viewport1.setTableColumns([
    { "ID": "Name", "Label": "Name", "Width": 150 },
    { "ID": "Value", "Label": "Value", "Type": "Slider", "Width": 100 }
]);

Viewport1.setTableCallback(inline function(event)
{
    Console.print(event.Type + " row: " + event.rowIndex);
});

Viewport1.setTableRowData([
    { "Name": "Item A", "Value": 0.5 },
    { "Name": "Item B", "Value": 0.8 }
]);
```

## Alternatives
ScriptPanel with custom paint routine for fully custom list/table rendering. ScriptFloatingTile with PresetBrowser content type for preset browsing use cases.

## Related Preprocessors
None.

## Diagrams

### viewport-modes
- **Brief:** ScriptedViewport Operating Modes
- **Type:** state
- **Description:** The ScriptedViewport has three mutually exclusive modes determined at init time. Default state is Viewport mode (plain scrollable container). Setting the useList property to true switches to List mode (selectable text list from Items property, value is row index). Calling setTableMode() in onInit switches to Table mode (interactive data table with configurable columns, value is row index or [column, row] array). Mode is fixed after onInit completes -- cannot be changed at runtime.

### table-setup
- **Brief:** Table Mode Setup Sequence
- **Type:** sequence
- **Description:** Table mode setup requires a mandatory call sequence during onInit: (1) setTableMode() to activate and configure table behavior, (2) setTableColumns() to define the column structure and cell types, (3) setTableCallback() to register the interaction handler. All three must be called in onInit in this order. After init, setTableRowData() is the only table method that can be called -- it populates or refreshes row data at any time and can be called repeatedly.
