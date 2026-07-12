BackgroundTask::runProcess(String command, var args, Function logFunction) -> undefined

Thread safety: UNSAFE -- stops any running thread, allocates ChildProcessData, starts a new high-priority thread that spawns an OS child process.
Spawns an OS child process on the background thread and streams output line by
line to the log callback. Both stdout and stderr are merged into a single
stream. The process respects shouldAbort() and is killed on abort.
Callback signature: f(BackgroundTask task, bool isFinished, var data)
Required setup:
  const var bt = Engine.createBackgroundTask("ProcessRunner");
Dispatch/mechanics:
  stopThread(timeout) -> new ChildProcessData(command, args, logFn)
    -> ThreadStarters::startHigh() -> run() calls childProcessData.run()
    -> reads output char by char, calls logFn(task, false, lineText) per line
    -> on exit: logFn(task, true, exitCode) then callFinishCallback(true, wasCancelled)
Pair with:
  shouldAbort -- checked during process output reading; kills process on abort
  setFinishCallback -- fires after process completes (same protocol as callOnBackgroundThread)
  setTimeOut -- configure thread stop timeout
Anti-patterns:
  - Do NOT pass args as a String when arguments contain spaces -- use an Array instead.
    String args are tokenized by spaces (quotes respected), Array preserves each element.
  - Do NOT assume data type without checking isFinished -- data is String (line text)
    during output and int (exit code) on completion
Source:
  ScriptingApiObjects.cpp  runProcess()
    -> ChildProcessData(command, args, logFn) with 3-arg WeakCallbackHolder
    -> ChildProcessData::run() spawns juce::ChildProcess with stdout+stderr
    -> reads char by char, calls callLog() on newlines
    -> checks parent.shouldAbort() in loop, kills process on abort
