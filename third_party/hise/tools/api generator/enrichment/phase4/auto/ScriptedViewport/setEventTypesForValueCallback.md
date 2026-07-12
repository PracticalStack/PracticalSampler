Specifies which table event types trigger the parent component's `setValue()` callback. This is the bridge between table interactions and the component value system, so it also enables undo support when `useUndoManager` is active.

Available event types:

| Event type | Description |
|------------|-------------|
| `Selection` | Fires when the row selection changes |
| `SingleClick` | Fires on a single click |
| `DoubleClick` | Fires on a double click |
| `ReturnKey` | Fires when the user confirms with Return |
| `SpaceKey` | Fires when the user triggers the focused row with Space |

By default, `SingleClick`, `DoubleClick`, `ReturnKey`, and `SpaceKey` trigger the value callback.

The stored value is either the selected row index or, in `MultiColumnMode`, a `[columnIndex, rowIndex]` array.

If you also use `setTableCallback()`, the callback event type becomes `SetValue` or `Undo` when the value changes through `setValue()` or undo/redo.
