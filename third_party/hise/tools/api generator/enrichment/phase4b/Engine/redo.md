Engine::redo() -> undefined

Thread safety: UNSAFE -- may dispatch to message thread via MessageManager::callAsync
Redoes the last undone action. Script transactions execute synchronously; other actions
dispatch to the message thread asynchronously.
Pair with:
  undo -- revert actions
  performUndoAction -- register undoable actions
Source:
  ScriptingApi.cpp  Engine::redo()
    -> checks %SCRIPT_TRANSACTION% marker -> sync or async redo
