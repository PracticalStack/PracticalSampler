ExpansionHandler::setErrorFunction(var newErrorFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder on the heap.
Sets a callback for expansion-related error and log messages. Fires on expansion
initialization failures, credential validation errors, and manual setErrorMessage()
calls.
Callback signature: f(String message, bool isCritical)
Pair with:
  setErrorMessage -- manually trigger the error callback
  setCredentials -- invalid credentials route errors through this callback
Source:
  ScriptExpansion.cpp:1207  setErrorFunction()
    -> creates WeakCallbackHolder(numExpectedArgs=1, but called with 2 args)
    -> logMessage() calls errorFunction.call(args, 2)
