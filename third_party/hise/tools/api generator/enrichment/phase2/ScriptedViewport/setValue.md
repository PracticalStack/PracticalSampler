## setValue

**Examples:**

```javascript:programmatic-cell-selection-in
// Title: Programmatic cell selection in a multi-column sorted table
// Context: After loading content, find the matching row in a sorted table
//          and select it. Use getOriginalRowIndex() to map between the
//          sorted display order and the original data order, then call
//          setValue([column, row]) to highlight the active cell.

const var table = Content.addViewport("DataTable", 0, 0);

// ... setTableMode with MultiColumnMode: true ...

var rowData = [];

// Select a specific item by its original data index
inline function selectByOriginalIndex(targetOriginalIndex)
{
    for (i = 0; i < rowData.length; i++)
    {
        if (table.getOriginalRowIndex(i) == targetOriginalIndex)
        {
            // Select column 1, display row i
            table.setValue([1, i]);

            // Scroll so the selected row is visible
            table.set("viewPositionY", (i + 1) / rowData.length);
            table.sendRepaintMessage();
            return;
        }
    }

    // No match -- deselect
    table.setValue([-1, -1]);
    table.sendRepaintMessage();
};

```
```json:testMetadata:programmatic-cell-selection-in
{
  "testable": false
}
```

**Pitfalls:**
- In `MultiColumnMode`, pass a `[column, row]` array. The column index refers to the column's position in the `setTableColumns()` array (0-based). Passing a plain integer selects a row without column tracking.
- After calling `setValue()` on a table that is not currently visible or whose LAF repaints are not triggered automatically, call `sendRepaintMessage()` to force a visual update.
