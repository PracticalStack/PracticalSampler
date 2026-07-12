# ScriptedViewport -- Project Context

## Project Context

### Real-World Use Cases
- **Sortable data browser with favorites**: A drum machine or sample-based plugin builds a multi-column table for browsing sounds or presets. Rows display names and metadata (BPM, category); a toggle-button column marks favorites. Column-header sorting lets users reorder by any text or numeric field. The table callback dispatches click/double-click events to load content, and `getOriginalRowIndex()` maps sorted display rows back to the underlying data array for mutations like toggling favorites.
- **Modulation matrix viewer**: A synthesizer uses table mode with Slider and Button cell types to display active modulation connections. Each row is a source-target pair with an intensity slider and an invert toggle. The table callback routes slider-drag events to the modulation engine and button-click events to connect/disconnect operations. Row data is rebuilt from the modulation model whenever connections change.
- **Scrollable child container**: A plugin with a sidebar of expandable sections (e.g., category lists, tag filters) wraps child panels in a plain viewport (no table mode). A factory function creates the viewport, attaches a scrollbar LAF, and parents a canvas panel inside it. The canvas panel resizes to fit its visible children, providing virtual scrolling over arbitrarily tall content.
- **Simple file selector**: A popup panel for selecting impulse responses uses a minimal two-column table with `MultiColumnMode` enabled and `HeaderHeight: 0`. The table displays file names in two columns side by side, and the callback loads the selected IR. This lightweight use shows that table mode scales down to simple selection lists.

### Complexity Tiers
1. **Plain viewport** (simplest): Use `Content.addViewport()` with default settings. Parent child panels inside it for a scrollable container. Only a `drawScrollbar` LAF and `scrollBarThickness` property are needed.
2. **Simple table**: Call `setTableMode()`, `setTableColumns()`, and `setTableCallback()` in onInit. Use Text and Button column types with a straightforward callback that switches on `event.Type`. Populate data with `setTableRowData()`.
3. **Interactive table with sorting and multi-column selection**: Enable `Sortable`, `MultiColumnMode`, and `MultiSelection` in the table metadata. Use `getOriginalRowIndex()` to map sorted indices back to data. Add column types including Slider and ComboBox. Route the table callback through a Broadcaster for event-bus integration. Customize all four table LAF functions.

### Practical Defaults
- Use `"RowHeight": 28` to `"RowHeight": 33` for comfortable touch and mouse targets in data browsers.
- Use `"HeaderHeight": 0` when column labels are not needed (e.g., simple selectors or single-purpose lists).
- Use `"ScrollOnDrag": true` for any table that may be used on touchscreens or with a pen tablet.
- Use `"MultiColumnMode": true` when you need `setValue([column, row])` for programmatic cell selection, which is essential for highlighting the currently-loaded item in a sorted table.
- Set `"scrollBarThickness"` to 8-10 pixels for a minimal scrollbar appearance alongside custom `drawScrollbar` LAF.
- Pass a Broadcaster directly to `setTableCallback()` instead of an inline function when the table events need to drive multiple listeners (e.g., sample loading, preview playback, UI state updates).

### Integration Patterns
- `ScriptedViewport.setTableCallback(broadcaster)` -> `Broadcaster.addListener()` -- Pass a Broadcaster as the table callback to fan out table events to multiple listeners without coupling the table to specific handlers.
- `ScriptedViewport.getOriginalRowIndex(displayIndex)` -> data array lookup -- When `Sortable: true`, the display order differs from the data array order. Use this after every callback that mutates the underlying data (favorites, deletion) to get the correct index.
- `ScriptedViewport.set("viewPositionY", normalized)` -> scroll to row -- Programmatically scroll to a specific row by computing `(rowIndex + 1) / totalRows` and setting the normalized scroll position.
- `ScriptedViewport.setLocalLookAndFeel(laf)` -> `drawTableRowBackground` + `drawTableCell` + `drawTableHeaderColumn` + `drawScrollbar` -- A complete table visual override requires registering all four table LAF functions plus the scrollbar function on a single LAF object attached to the viewport.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `obj.rowIndex` directly to index into the data array when sorting is enabled | Using `viewport.getOriginalRowIndex(obj.rowIndex)` to map back to the original data index | When the table is sorted, display indices no longer match the original array positions. Mutations (favorites, deletion) applied to the wrong index corrupt data silently. |
| Passing a Broadcaster as callback but forgetting to pass it as the function argument to `setTableCallback()` | `vp.setTableCallback(myBroadcaster);` -- Broadcasters are valid callback targets | The table callback can accept a Broadcaster directly. When it fires, the Broadcaster's `sendMessage()` is called with the event object, fanning it out to all registered listeners. |
| Setting `viewPositionY` to a pixel value | Setting `viewPositionY` to a normalized 0.0-1.0 value | The scroll position properties are normalized, not pixel-based. Use `(targetRow + 1) / totalRows` to scroll to a specific row. |
