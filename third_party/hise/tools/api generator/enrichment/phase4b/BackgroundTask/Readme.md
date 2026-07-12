BackgroundTask (object)
Obtain via: Engine.createBackgroundTask(name)

Task handle for running long operations on a background thread with progress,
abort, and process spawning. Wraps a JUCE Thread with progress reporting,
status messages, abort signaling, thread-safe property storage, and a finish
callback. Automatically stops on script recompilation.

Complexity tiers:
  1. Voice-safe loading: killVoicesAndCall. Safe execution of functions that
     modify audio state. No progress or abort -- just voice-safe dispatch.
  2. Background work with progress: callOnBackgroundThread, shouldAbort,
     setProgress. Optionally + setForwardStatusToLoadingThread to reuse the
     sample loading overlay. Most common tier.
  3. Shared task with abort-restart: + sendAbortSignal, setTimeOut,
     setFinishCallback. Single BackgroundTask reused across multiple triggers,
     cancelling in-progress work before restarting.

Practical defaults:
  - Use setForwardStatusToLoadingThread(true) when the task takes more than a
    second -- provides a free progress UI via the built-in loading overlay.
  - Use setTimeOut(4000) or higher for file scanning tasks. The default 500ms
    is too short for I/O-heavy operations with longer gaps between
    shouldAbort() calls.
  - Create one shared BackgroundTask per subsystem rather than a new task per
    operation. Reuse the same instance and let abort-restart handle
    cancellation.

Common mistakes:
  - Long loop without shouldAbort() -- script engine timeout is not extended
    and the task cannot be cancelled. HISE IDE warns if gap exceeds timeout.
  - sendAbortSignal(true) from inside the background function -- deadlock
    (thread waiting for itself). Detected and throws script error.
  - Creating a new BackgroundTask for every operation -- each is a dedicated
    JUCE thread. Reuse one instance per subsystem.
  - Using callOnBackgroundThread when modifying processor state (bypass,
    attributes, routing) -- use killVoicesAndCall instead, which suspends
    audio first.
  - Default 500ms timeout for I/O-heavy tasks -- triggers spurious warnings.
    Use setTimeOut(2000) or higher for file scanning.

Example:
  // Create a background task and run a function on it
  const var bt = Engine.createBackgroundTask("MyTask");

  bt.setFinishCallback(function(isFinished, wasCancelled)
  {
      if (isFinished && !wasCancelled)
          Console.print("Task completed");
  });

  bt.callOnBackgroundThread(function(task)
  {
      for (var i = 0; i < 100; i++)
      {
          if (task.shouldAbort())
              return;

          task.setProgress(i / 100.0);
          task.setStatusMessage("Processing " + i);
      }
  });

Methods (14):
  callOnBackgroundThread      getProgress
  getProperty                 getStatusMessage
  killVoicesAndCall           runProcess
  sendAbortSignal             setFinishCallback
  setForwardStatusToLoadingThread  setProgress
  setProperty                 setStatusMessage
  setTimeOut                  shouldAbort
