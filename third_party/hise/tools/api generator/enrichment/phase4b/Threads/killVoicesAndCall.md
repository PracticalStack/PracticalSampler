Threads::killVoicesAndCall(var functionToExecute) -> Integer

Thread safety: UNSAFE -- Suspends audio processing, kills all voices, acquires ScriptLock, and defers execution to the SampleLoadingThread.
Suspends audio processing, kills all active voices, and executes the given
function on the SampleLoadingThread with the ScriptLock held. Returns true if
executed synchronously, false if deferred.
Callback signature: functionToExecute()

Dispatch/mechanics:
  WeakCallbackHolder(functionToExecute, argCount=0)
    -> KillStateHandler::killVoicesAndCall(processor, lambda, SampleLoadingThread)
      -> lambda acquires LockHelpers::SafeLock(ScriptLock)
      -> callSync(nullptr, 0, nullptr)

Anti-patterns:
  - Do NOT treat return value false as failure -- false means the function was
    queued for deferred execution on the loading thread, not an error
  - Do NOT use `this` context inside the callback -- executes on loading thread
    where `this` may not be valid
  - Do NOT call setBypassed()/setAttribute() on multiple processors without
    wrapping in killVoicesAndCall -- causes audio glitches

Source:
  ScriptingApi.cpp  killVoicesAndCall()
    -> WeakCallbackHolder wc(getScriptProcessor(), this, functionToExecute, 0)
    -> getKillStateHandler().killVoicesAndCall(processor, lambda, TargetThreadId::SampleLoadingThread)
    -> lambda: LockHelpers::SafeLock(ScriptLock) -> copy.callSync(nullptr, 0, nullptr)
