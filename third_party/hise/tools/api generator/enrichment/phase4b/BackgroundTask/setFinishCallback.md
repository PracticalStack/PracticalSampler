BackgroundTask::setFinishCallback(Function newFinishCallback) -> undefined

Thread safety: UNSAFE -- allocates WeakCallbackHolder, sets source tracking properties.
Sets the callback that fires when a background task starts and finishes. Called
with (false, false) on start, (true, false) on normal completion, (true, true)
on abort. The BackgroundTask is available as `this` inside the callback.
Callback signature: f(bool isFinished, bool wasCancelled)
Dispatch/mechanics:
  new WeakCallbackHolder(f, 2) -> setThisObject(this)
    -> addAsSource("onTaskFinished")
Pair with:
  callOnBackgroundThread -- triggers finish callback on start and completion
  runProcess -- triggers finish callback on start and completion
Anti-patterns:
  - Do NOT expect this to fire from killVoicesAndCall -- only callOnBackgroundThread
    and runProcess trigger the finish callback
Source:
  ScriptingApiObjects.cpp  setFinishCallback()
    -> WeakCallbackHolder(f, 2) + incRefCount()
    -> setThisObject(this) + addAsSource("onTaskFinished")
