Sets this component's value in the Values tree using the global undo manager, without triggering the control callback. Call `changed()` afterward to fire the callback.

> [!Warning:Always uses global undo manager] This method uses the global undo manager regardless of the component's `useUndoManager` property. Undo will work even if `useUndoManager` is false on the component.
