ErrorHandler::getCurrentErrorLevel() -> int

Thread safety: SAFE
Returns the state constant of the highest-priority (lowest-numbered) active error,
or -1 if no errors are active.

Anti-patterns:
  - Do NOT assume this returns the most recently received error -- it returns the
    lowest-numbered active state. If both SamplesNotFound (10) and IllegalBufferSize (11)
    are active, this returns 10 regardless of arrival order.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::getCurrentErrorLevel()
    -> iterates errorStates bits from 0 upward, returns first set bit
