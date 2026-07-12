# Table -- Project Context

## Project Context

### Real-World Use Cases
- **Velocity response curve**: A piano plugin places a ScriptTable on the interface and obtains its Table data handle via `registerAtParent()`. In the `onNoteOn` callback, `getTableValueNormalised()` remaps incoming MIDI velocity through the user-editable curve before forwarding the note. This lets the player shape the instrument's dynamic response without touching the sample map.
- **Sample velocity zone redistribution**: A backend sample editing tool uses the Table curve to remap velocity zone boundaries across a set of samples. The curve controls how velocity layers are distributed - a convex curve concentrates layers in the low-velocity range, a concave curve in the high range.

### Complexity Tiers
1. **UI-driven curve query** (most common): `ScriptTable` UI component + `registerAtParent(0)` to get a Table handle, then `getTableValueNormalised()` in a MIDI callback. No programmatic point setup needed - the user edits the curve visually.
2. **Programmatic curve setup**: `Engine.createAndRegisterTableData()` or `registerAtParent()` followed by `setTablePointsFromArray()` to define curves from code (e.g., loading curve presets, generating mathematical shapes).
3. **Linked tables with callbacks**: `linkTo()` to share data between multiple Table handles, `setContentCallback()` and `setDisplayCallback()` for reactive UI synchronization when tables are edited or queried.

### Practical Defaults
- Use `ScriptTable` + `registerAtParent(0)` when the curve should be user-editable. Use `Engine.createAndRegisterTableData()` when the table is purely programmatic with no UI.
- The default linear ramp (0,0) to (1,1) after `reset()` is identity - input equals output. This is a good starting point for velocity curves where the user can then shape the response.
- Normalize MIDI values to 0.0-1.0 before querying: `table.getTableValueNormalised(Message.getVelocity() / 127.0)`, then scale back to the target range.

### Integration Patterns
- `ScriptTable.registerAtParent(0)` -> `Table.getTableValueNormalised()` in `onNoteOn` - the primary pattern for user-editable velocity/modulation curves.
- `Table.getTableValueNormalised()` -> `Message.setVelocity()` - velocity remapping pipeline where the table output scales the MIDI velocity.
- `Table.reset()` followed by re-application logic - reset-and-reapply pattern for restoring default curves in tools.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `table.getTableValueNormalised(Message.getVelocity())` passing raw 0-127 | `table.getTableValueNormalised(Message.getVelocity() / 127.0)` normalizing to 0.0-1.0 | The method expects normalized 0.0-1.0 input. Raw MIDI values exceed 1.0 and return the last table value for all velocities above 1. |
