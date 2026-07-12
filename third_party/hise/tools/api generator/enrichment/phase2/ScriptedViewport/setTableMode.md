## setTableMode

**Examples:**

```javascript:multi-column-browser-table-with
// Title: Multi-column browser table with sorting and selection
// Context: A sortable data browser that supports programmatic cell selection
//          and touch-friendly scrolling. HeaderHeight: 0 hides the column
//          labels when the context makes them unnecessary.

const var browser = Content.addViewport("Browser", 0, 0);
browser.set("width", 600);
browser.set("height", 400);

browser.setTableMode({
    "RowHeight": 30,
    "HeaderHeight": 0,
    "Sortable": true,
    "MultiColumnMode": true,
    "MultiSelection": true,
    "ScrollOnDrag": true
});

browser.setTableColumns([
    {"ID": "Name", "Label": "Name", "MinWidth": 150},
    {"ID": "Value", "Label": "Value", "MinWidth": 100}
]);

var rowData = [];
for (i = 0; i < 5; i++)
    rowData.push({"Name": "Row " + i, "Value": String.fromCharCode(65 + i)});

browser.setTableRowData(rowData);

browser.set("saveInPreset", false);
browser.setValue([0, 2]);
```
```json:testMetadata:multi-column-browser-table-with
{
  "testable": true,
  "verifyScript": {
    "type": "REPL",
    "expression": "browser.getValue()",
    "value": [
      0,
      2
    ]
  }
}
```


```javascript:sortable-preset-table-with
// Title: Sortable preset table with visible column headers
// Context: A preset browser where users click column headers to sort by
//          name, category, or BPM. The header row needs a visible height
//          for the sort indicator to be meaningful.

const var presetTable = Content.addViewport("PresetTable", 0, 0);
presetTable.set("width", 500);
presetTable.set("height", 350);

presetTable.setTableMode({
    "RowHeight": 33,
    "HeaderHeight": 30,
    "Sortable": true,
    "MultiSelection": false,
    "ScrollOnDrag": true
});

```
```json:testMetadata:sortable-preset-table-with
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "presetTable.getWidth()", "value": 500},
    {"type": "REPL", "expression": "presetTable.getHeight()", "value": 350}
  ]
}
```

**Pitfalls:**
- When `MultiColumnMode` is enabled, `setValue()` accepts a `[column, row]` array to select a specific cell programmatically. This is essential for highlighting the currently-active item in a sorted table, but the column index refers to the column's position in the `setTableColumns()` array (0-based), not a column ID string.
