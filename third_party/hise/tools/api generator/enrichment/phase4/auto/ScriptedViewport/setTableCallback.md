Registers a callback function for all table interactions. The callback receives a single event object. Must be called in onInit after `setTableMode()`.

**Event object properties:**

| Property | Type | Description |
|----------|------|-------------|
| `Type` | String | The event type (see table below) |
| `rowIndex` | int | Row index in current display order, or -1 for background clicks |
| `columnID` | String | The `ID` from the column definition |
| `value` | var | Depends on event type (see table below) |

**Event types:**

| Type | Trigger | value |
|------|---------|-------|
| `Click` | Single click on a cell | Full row data object |
| `DoubleClick` | Double click on a cell | Full row data object |
| `Selection` | Row selection changes | Full row data object |
| `ReturnKey` | Return key on focused row | Full row data object |
| `SpaceKey` | Space key on focused row (requires `ProcessSpaceKey`) | Full row data object |
| `DeleteRow` | Delete key on focused row | Full row data object |
| `Slider` | Slider cell value changes | Numeric slider value |
| `Button` | Button cell click/toggle | Toggle state (bool) |
| `ComboBox` | ComboBox selection changes | Selected item (per `ValueMode`) |
| `SetValue` | Programmatic `setValue()` call | `[column, row]` array |
