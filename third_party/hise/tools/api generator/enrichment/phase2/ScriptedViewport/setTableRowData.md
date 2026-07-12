## setTableRowData

**Examples:**

```javascript:dynamic-row-data-with
// Title: Dynamic row data with runtime refresh for a preset browser
// Context: The row data array is rebuilt whenever the user changes a
//          filter or toggles a favorite. Each row object's keys match
//          the column IDs defined in setTableColumns(). After updating
//          data, re-select the current item by walking the sorted table.

const var presetTable = Content.addViewport("PresetTable", 0, 0);

// ... setTableMode / setTableColumns / setTableCallback in onInit ...

// Build row data from file list
inline function buildRowData(fileList)
{
    local rows = [];

    for (f in fileList)
    {
        rows.push({
            "Name": f.toString(0),
            "Category": f.getParentDirectory().toString(0),
            "BPM": "120",
            "FullPath": f.toString(1),
            "Favorite": false
        });
    }

    return rows;
};

// Refresh the table at runtime (called from filter/search callbacks)
inline function refreshTable(fileList)
{
    local rows = buildRowData(fileList);
    presetTable.setTableRowData(rows);
};

```
```json:testMetadata:dynamic-row-data-with
{
  "testable": false
}
```

```javascript:building-modulation-matrix-rows
// Title: Building modulation matrix rows from a data model
// Context: Each row represents a modulation connection. Slider cells
//          receive a data object with Value, min, max, and style
//          properties instead of a plain number, enabling the table
//          to render the slider with the correct range.

// Rebuild rows from the modulation model
inline function rebuildModRows()
{
    local connections = [];  // from modulation engine
    local rows = [];

    for (c in connections)
    {
        rows.push({
            "Source": c.sourceName,
            "Target": c.targetName,
            "Intensity": {
                "Value": c.intensity,
                "min": -1.0,
                "max": 1.0,
                "style": "Knob"
            },
            "Mode": c.valueMode,
            "Delete": false
        });
    }

    modTable.setTableRowData(rows);
};

```
```json:testMetadata:building-modulation-matrix-rows
{
  "testable": false
}
```

**Pitfalls:**
- The data is cloned internally when passed to `setTableRowData()`. Modifying the original array afterwards has no effect on the displayed table. Always call `setTableRowData()` again after changing the data.
- For Slider columns, the row data value can be either a plain number or a data object with `Value`, `min`, `max`, `style`, and `suffix` properties. The object form lets you configure each slider cell individually (e.g., bipolar range for some rows, unipolar for others).
