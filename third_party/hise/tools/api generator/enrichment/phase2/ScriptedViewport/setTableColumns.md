## setTableColumns

**Examples:**

```javascript:preset-browser-columns
// Title: Preset browser with play button, sortable text columns, and hidden data
// Context: A preset browser table with a momentary play button, text columns
//          for name and category, and a Hidden column storing the file path.
//          The Hidden column is not displayed but its data is accessible in
//          the callback via event.value.FullPath.

const var presetTable = Content.addViewport("PresetTable", 0, 0);
presetTable.setTableMode({ "RowHeight": 33, "HeaderHeight": 30, "Sortable": true });

presetTable.setTableColumns([
    { "ID": "Play",      "Label": "",         "Type": "Button", "MinWidth": 32,
      "Toggle": false,   "Focus": false,      "Text": "Play" },
    { "ID": "Name",      "Label": "Name",     "MinWidth": 200 },
    { "ID": "Category",  "Label": "Category", "MinWidth": 150 },
    { "ID": "BPM",       "Label": "BPM",      "MinWidth": 50 },
    { "ID": "FullPath",  "Type": "Hidden",    "MinWidth": 1 }
]);
```
```json:testMetadata:preset-browser-columns
{
  "testable": false
}
```

```javascript:modulation-matrix-columns
// Title: Modulation matrix with mixed interactive cell types
// Context: A modulation connection list where each row represents one
//          source-target pair. Slider cells control intensity, ComboBox
//          cells select the value mode, and Button cells provide
//          navigation and deletion.

const var modTable = Content.addViewport("ModTable", 0, 0);
modTable.setTableMode({ "RowHeight": 32, "HeaderHeight": 32 });

modTable.setTableColumns([
    { "ID": "Source",     "Type": "Text",     "MinWidth": 150 },
    { "ID": "ShowSource", "Type": "Button",   "Label": "", "MinWidth": 32,
      "Text": "goto" },
    { "ID": "Target",     "Type": "Text",     "MinWidth": 170 },
    { "ID": "ShowTarget", "Type": "Button",   "Label": "", "MinWidth": 32,
      "Text": "goto" },
    { "ID": "Intensity",  "Type": "Slider",   "MinWidth": 110 },
    { "ID": "Mode",       "Type": "ComboBox", "MinWidth": 80,
      "Text": "Default",  "ValueMode": "Text" },
    { "ID": "Inverted",   "Type": "Button",   "MinWidth": 80,
      "Toggle": true,     "Text": "Inverted" },
    { "ID": "Delete",     "Type": "Button",   "Label": "", "MinWidth": 32,
      "Toggle": false,    "Text": "Delete" }
]);
```
```json:testMetadata:modulation-matrix-columns
{
  "testable": false
}
```

**Pitfalls:**
- Set `"Focus": false` on Button columns that should not participate in arrow-key navigation. Without this, the focus order includes every column, making keyboard navigation slow when there are many action buttons.
- The `"Text"` property serves double duty: it is the button label for Button columns and the placeholder text for ComboBox columns. A Button column with `"Text": "Fav"` labels the button "Fav" in every row.
