ErrorHandler::setErrorCallback(var errorCallback) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder with heap allocation.
Registers a callback that fires when an error state changes. The callback receives
the highest-priority active error state and its resolved message. Fires on the
message thread with high priority (before regular script callbacks). Replaces any
previously registered callback.
Callback signature: f(int state, String message)

Required setup:
  const var eh = Engine.createErrorHandler();

Dispatch/mechanics:
  isJavascriptFunction(errorCallback) -> new WeakCallbackHolder(2 args)
    -> setHighPriority() -> addAsSource("onErrorCallback")

Anti-patterns:
  - [BUG] Passing a non-function value (including false) silently does nothing --
    it does not clear the existing callback. There is no way to unregister a
    callback once registered.
  - Do NOT create ErrorHandler without calling setErrorCallback() -- creating it
    disables the default overlay, so without a callback errors become invisible.

Source:
  ScriptingApiObjects.cpp  ScriptErrorHandler::setErrorCallback()
    -> WeakCallbackHolder(getScriptProcessor(), this, errorCallback, 2)
    -> callback.incRefCount() -> callback.setHighPriority()
