Engine::undo() -> undefined

Thread safety: UNSAFE -- may dispatch to message thread asynchronously
Reverts last undoable action. Script transactions execute synchronously; other
actions dispatch to message thread via MessageManager::callAsync.
Anti-patterns:
  - Non-script undo is async -- do NOT read state immediately after calling undo()
Pair with:
  redo -- redo last undone action
  performUndoAction -- register undoable actions
Source:
  ScriptingApi.cpp  Engine::undo()
    -> checks SCRIPT_TRANSACTION marker -> sync or MessageManager::callAsync
