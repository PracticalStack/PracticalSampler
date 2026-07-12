ErrorHandler::getNumActiveErrors() -> int

Thread safety: SAFE
Returns the number of currently active error states. Returns 0 when no errors
are active.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::getNumActiveErrors()
    -> counts set bits in errorStates (BigInteger)
