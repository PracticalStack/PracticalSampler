ExpansionHandler::setErrorMessage(var errorMessage) -> undefined

Thread safety: UNSAFE -- dispatches error callback via WeakCallbackHolder.
Manually triggers the error function callback with the given message and
isCritical=false. Useful for injecting custom warnings into the expansion error
handling pipeline.
Pair with:
  setErrorFunction -- must be set first or message is silently discarded
Anti-patterns:
  - Do NOT call without setting an error function first -- message is silently
    discarded with no indication
Source:
  ScriptExpansion.cpp  setErrorMessage()
    -> logMessage(errorMessage.toString(), false)
    -> errorFunction.call(args, 2)
