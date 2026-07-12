# setValue | SAFE

Sets the component's value. Thread-safe - can be called from any thread; the UI update happens asynchronously. Propagates the value to all linked component targets.

```
setValue(var newValue)
```

## Anti-Patterns

- Do NOT pass a String value. Reports a script error.
- If called during `onInit`, the value will NOT be restored after recompilation.

## Pair With

- `getValue()` - read the current value
- `setValueWithUndo()` - set value with undo support
- `setPanelValueWithUndo()` - panel-specific undo with old/new value pair

## Source

`ScriptingApiContent.h` line ~1734
