Performs a bulk write and records it in the undo history.

Use this for user-facing edit commands where undo and redo behaviour must be preserved.

> [!Warning:Always emits change notification] This method emits change notification even when `setAllValueChangeCausesCallback(0)` is active.
