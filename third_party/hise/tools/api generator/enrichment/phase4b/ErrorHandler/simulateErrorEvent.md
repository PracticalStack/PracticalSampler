ErrorHandler::simulateErrorEvent(int state) -> undefined

Thread safety: UNSAFE -- triggers error callback synchronously, involving String
construction via getErrorMessage().
Simulates an error event as if it came from the HISE system. Sets the specified
state as active and fires the error callback with the highest-priority active error.
Useful for testing error handling UI during development.

Dispatch/mechanics:
  errorStates.setBit(state, true) -> sendErrorForHighestState()
    -> callback(getCurrentErrorLevel(), getErrorMessage())

Pair with:
  setErrorCallback -- must register callback first to observe simulated events
  clearErrorLevel -- clear simulated states after testing

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::simulateErrorEvent()
    -> overlayMessageSent(state, "") reuse path -> sendErrorForHighestState()
