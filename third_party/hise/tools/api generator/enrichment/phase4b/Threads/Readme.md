Threads (namespace)

Thread identity queries, lock state inspection, and safe audio-suspended
function execution. Exposes HISE's KillStateHandler threading system to
HiseScript for introspection and controlled audio suspension.

Constants:
  ThreadId:
    UI = 0             Message/UI thread identifier
    Scripting = 1      Scripting thread identifier
    Loading = 2        Sample loading thread identifier
    Audio = 4          Audio thread identifier (also used for audio export thread)
    Unknown = 5        Unknown or unrecognized thread
    Free = 6           No thread / unlocked state

Complexity tiers:
  1. Constants only: Threads.Audio, Threads.Scripting, etc. as lock identifiers
     for BackgroundTask.lock(). Most common usage -- no methods called directly.
  2. Thread-safe state modification: killVoicesAndCall, isAudioRunning. Suspend
     audio to safely reconfigure processors.
  3. Diagnostic and profiling: getCurrentThread, isLocked, getLockerThread,
     isLockedByCurrentThread, toString. + startProfiling (requires
     HISE_INCLUDE_PROFILING_TOOLKIT).

Practical defaults:
  - Use BackgroundTask.killVoicesAndCall(fn) for operations inside a larger
    background workflow with progress tracking. Use Threads.killVoicesAndCall(fn)
    for simple one-shot state modifications from UI callbacks.
  - Use Threads.Scripting as the lock type for BackgroundTask.lock() when the
    background thread reads or writes script-accessible state.

Common mistakes:
  - Calling setBypassed()/setAttribute() on multiple processors from a UI
    callback without suspending audio -- causes brief audio glitches. Wrap bulk
    reconfiguration in Threads.killVoicesAndCall().
  - Treating killVoicesAndCall return value false as failure -- false means
    deferred execution (queued for loading thread), not an error.
  - Using `this` context inside the killVoicesAndCall callback -- the callback
    executes on the loading thread where `this` may not be valid.

Example:
  // Query current thread identity
  var threadId = Threads.getCurrentThread();

  if (threadId == Threads.Audio)
      Console.print("Running on audio thread");

Methods (10):
  getCurrentThread          getCurrentThreadName
  getLockerThread           isAudioRunning
  isCurrentlyExporting      isLocked
  isLockedByCurrentThread   killVoicesAndCall
  startProfiling            toString
