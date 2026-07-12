ErrorHandler::clearAllErrors() -> undefined

Thread safety: SAFE
Clears all active error states at once. After this call, getCurrentErrorLevel()
returns -1 and getNumActiveErrors() returns 0. Does not fire the error callback.

Anti-patterns:
  - Do NOT rely on the error callback to update UI after clearing -- the callback
    does not fire. Explicitly update error UI (e.g., hide overlay) after calling.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::clearAllErrors()
    -> errorStates.clear() (BigInteger reset)
