BackgroundTask::callOnBackgroundThread(Function backgroundTaskFunction) -> undefined

Thread safety: UNSAFE -- stops any running thread (blocking up to timeout ms), allocates WeakCallbackHolder, starts new high-priority thread.
Starts the given function on a dedicated high-priority background thread. The
function receives the BackgroundTask as its single argument for shouldAbort()
and setProgress() calls. If a task is already running, it is stopped first.
Callback signature: f(BackgroundTask task)
Required setup:
  const var bt = Engine.createBackgroundTask("MyTask");
Dispatch/mechanics:
  callFinishCallback(false, false) -> stopThread(timeout) -> new WeakCallbackHolder(f, 1)
    -> ThreadStarters::startHigh() -> run() calls f(task) via callSync()
    -> callFinishCallback(true, threadShouldExit()) on completion
Pair with:
  shouldAbort -- must call regularly in loops to enable cancellation and extend timeout
  setFinishCallback -- to observe task start/completion/cancellation
  setTimeOut -- configure thread stop timeout before starting
Anti-patterns:
  - Do NOT start a new task without considering the blocking cost -- if the old task
    does not check shouldAbort(), starting a new task blocks for the full timeout duration
Source:
  ScriptingApiObjects.cpp:7716  callOnBackgroundThread()
    -> callFinishCallback(false, false)
    -> stopThread(timeOut)
    -> WeakCallbackHolder(f, 1) + addAsSource("backgroundFunction")
    -> ThreadStarters::startHigh(this)
