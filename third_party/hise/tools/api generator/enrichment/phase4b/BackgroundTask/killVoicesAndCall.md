BackgroundTask::killVoicesAndCall(Function loadingFunction) -> Integer

Thread safety: UNSAFE -- stops any running background thread, then uses KillStateHandler to kill voices and execute on the sample loading thread.
Kills all active voices and executes the function on the sample loading thread.
The function takes zero arguments and has no access to the BackgroundTask for
progress or abort checking. Returns true if the operation was queued.
Callback signature: f()
Required setup:
  const var bt = Engine.createBackgroundTask("Loader");
Dispatch/mechanics:
  stopThread(timeout) -> WeakCallbackHolder(f, 0)
    -> KillStateHandler::killVoicesAndCall(f, SampleLoadingThread)
    -> audio suspended, f() runs via callSync(nullptr, 0)
Pair with:
  callOnBackgroundThread -- alternative for tasks that do not need voice-safe execution
Anti-patterns:
  - Do NOT expect the finish callback to fire -- killVoicesAndCall does not trigger it.
    Use callOnBackgroundThread if you need finish callback notifications.
  - Do NOT use for tasks that need progress reporting -- the function receives no
    task argument. Use callOnBackgroundThread instead.
  - Do NOT use for general background work -- use only when modifying audio-thread-owned
    state (bypass, attributes, routing, purge)
Source:
  ScriptingApiObjects.cpp  killVoicesAndCall()
    -> WeakCallbackHolder(f, 0) with WeakReference safety
    -> KillStateHandler::killVoicesAndCall(processor, lambda, SampleLoadingThread)
