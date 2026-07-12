Reverts the last undoable action. Script undo actions (created via `Engine.performUndoAction()`) execute synchronously; other undo actions are dispatched asynchronously to the UI thread.

> [!Warning:State not immediately updated after call] Non-script undo operations complete after `undo()` returns. Do not read state immediately after calling `undo()` and expect it to reflect the reverted value.