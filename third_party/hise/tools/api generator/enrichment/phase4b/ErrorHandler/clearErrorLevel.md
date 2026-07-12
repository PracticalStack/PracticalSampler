ErrorHandler::clearErrorLevel(int stateToClear) -> undefined

Thread safety: UNSAFE -- triggers error callback synchronously when other errors
remain, involving String construction via getErrorMessage().
Clears a single error state by unsetting its bit. If other errors remain, the
callback fires immediately with the next highest-priority active error, enabling
cascading error resolution.

Dispatch/mechanics:
  clearBit(stateToClear) -> if errorStates not zero:
    sendErrorForHighestState() -> callback(getCurrentErrorLevel(), getErrorMessage())

Pair with:
  setErrorCallback -- callback fires on remaining errors after clearing
  getCurrentErrorLevel -- check which error is now active after clearing

Anti-patterns:
  - Do NOT assume the callback fires when the last error is cleared -- it does not.
    Check getNumActiveErrors() == 0 explicitly after calling to handle the "all clear"
    transition.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::clearErrorLevel()
    -> errorStates.clearBit(stateToClear)
    -> if (!errorStates.isZero()) sendErrorForHighestState()
