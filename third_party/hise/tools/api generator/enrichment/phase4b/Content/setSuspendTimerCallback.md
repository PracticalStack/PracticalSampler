Content::setSuspendTimerCallback(Function suspendFunction) -> undefined

Thread safety: UNSAFE -- creates a WeakCallbackHolder (heap allocation).
Registers a callback called when panel timers are suspended or resumed (e.g., when the
plugin window is hidden or shown). The callback receives a single boolean: true when
timers should be suspended, false when they should be resumed.
Callback signature: f(bool shouldBeSuspended)

Dispatch/mechanics:
  Validates via HiseJavascriptEngine::isJavascriptFunction
  Stores as WeakCallbackHolder with 1 argument
  Called by suspendPanelTimers(bool) which also iterates all ScriptPanels

Anti-patterns:
  - If the argument is not a valid JavaScript function, the method silently does
    nothing -- no error reported, any previously registered callback remains active.

Source:
  ScriptingApiContent.cpp:9127  Content::setSuspendTimerCallback()
    -> WeakCallbackHolder construction
    -> called from suspendPanelTimers(bool)
