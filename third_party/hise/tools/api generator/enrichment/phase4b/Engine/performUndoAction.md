Engine::performUndoAction(JSON thisObject, Function undoAction) -> Integer

Thread safety: UNSAFE -- creates ScriptUndoableAction on heap, UndoManager registration
Registers and performs a scriptable undo action. Callback receives false on perform,
true on undo. Thread-aware: synchronous on scripting thread, async on message thread.
Callback signature: undoAction(bool isUndo)
Pair with:
  undo/redo -- navigate undo history
  clearUndoHistory -- clear all actions
Source:
  ScriptingApi.cpp  Engine::performUndoAction()
    -> new ScriptUndoableAction(callback) -> UndoManager::perform()
