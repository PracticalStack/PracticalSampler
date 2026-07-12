Sets the component's value. In table mode with `MultiColumnMode` enabled, passing a 2-element array `[column, row]` triggers the table's selection callback. In list mode, pass an integer row index to select a row.

> [!Warning:MultiColumn mode uses column-row array] In `MultiColumnMode`, pass a `[column, row]` array. The column index refers to the column's position in the `setTableColumns()` array (0-based). Passing a plain integer selects a row without column tracking.
