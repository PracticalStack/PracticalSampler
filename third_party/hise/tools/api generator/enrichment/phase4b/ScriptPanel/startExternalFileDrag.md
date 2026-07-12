# startExternalFileDrag | UNSAFE

Initiates an OS-level file drag operation from this panel. Accepts a string file path, File object, or array of either. The optional finish callback is called when the drag completes.

```
startExternalFileDrag(var fileToDrag, int moveOriginal, Function finishCallback)
```

### Callback Signature

```
finishCallback()
```

## Dispatch / Mechanics

Calls JUCE's `DragAndDropContainer::performExternalDragDropOfFiles`. On Windows the drag operation runs synchronously; on other platforms it is deferred via `MessageManager::callAsync`.

## Pair With

- `startInternalDrag()` - drag within HISE UI instead of OS-level
- `setFileDropCallback()` - receive file drops on another panel

## Source

`ScriptingApiContent.cpp` line ~4220
