# startInternalDrag | UNSAFE

Initiates an internal HISE UI drag operation from this panel. Sends a `DragAction::Start` to the Content's RebuildListener system with the specified drag data.

```
startInternalDrag(var dragData)
```

## Pair With

- `startExternalFileDrag()` - OS-level file drag instead of internal
- `setDraggingBounds()` - constrain the panel's drag area

## Source

`ScriptingApiContent.cpp` line ~4220
