Engine::clearUndoHistory() -> undefined

Thread safety: UNSAFE -- calls UndoManager::clearUndoHistory() (deallocates stored actions)
Clears the undo history. Throws a script error if called while an undo or redo
operation is in progress.
Anti-patterns:
  - Do NOT call during an undo/redo callback -- throws a script error
Pair with:
  undo/redo -- navigate undo history
  performUndoAction -- register undoable actions
Source:
  ScriptingApi.cpp  Engine::clearUndoHistory()
    -> checks isPerformingUndoRedo() -> UndoManager::clearUndoHistory()
