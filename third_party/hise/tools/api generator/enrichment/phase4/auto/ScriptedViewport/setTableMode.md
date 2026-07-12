Activates table mode and configures table-wide behaviour. Must be called in onInit before `setTableColumns()` and `setTableCallback()`.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `RowHeight` | int | 20 | Height of each row in pixels |
| `HeaderHeight` | int | 24 | Height of the table header in pixels |
| `Sortable` | bool | false | Enables column header click-to-sort |
| `MultiColumnMode` | bool | false | Enables `[column, row]` value tracking |
| `MultiSelection` | bool | false | Enables multiple row selection |
| `ScrollOnDrag` | bool | false | Enables scroll-on-drag for touch devices |
| `ProcessSpaceKey` | bool | false | Enables SpaceKey event handling |
| `CallbackOnSliderDrag` | bool | true | If true, slider cells fire callbacks during drag; if false, only on release |
| `SliderRangeIdSet` | String | `"scriptnode"` | Which property name set to use for slider range values |

> [!Warning:MultiColumn setValue takes column-row array] When `MultiColumnMode` is enabled, `setValue()` accepts a `[column, row]` array to select a specific cell programmatically. The column index refers to the column's position in the `setTableColumns()` array (0-based), not a column ID string.
