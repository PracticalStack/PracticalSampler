Removes all child components from this component. Respects the undo manager if `useUndoManager` is true on this component.

> [!Warning:Removal is deferred] Children are not removed immediately when this method returns. `getNumChildComponents()` may still return the old count until the deferred operation completes.
