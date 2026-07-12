# setValueWithUndo | UNSAFE

Sets the value through the undo manager, creating an `UndoableControlEvent`. Intended for user-initiated value changes, not programmatic updates.

```
setValueWithUndo(var newValue)
```

## Anti-Patterns

- Do NOT call this from `onControl` callbacks. Use `setValue()` for programmatic changes.

## Pair With

- `setValue()` - set value without undo tracking
- `setPanelValueWithUndo()` - panel-specific undo that takes old value, new value, and action name

## Source

`ScriptingApiContent.h` line ~1734
