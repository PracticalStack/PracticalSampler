Removes this component from its parent. Before removal, clears the value callback and recursively removes all descendant values from the Values tree.

> [!Warning:Removal is deferred] The component is still in the tree immediately after this call returns. Reads from the tree may reflect the old state until the deferred operation completes.

> [!Warning:Uses parent's undo manager] Undo behaviour for the removal is controlled by the parent's `useUndoManager` property, not this component's.
