Threads::getCurrentThread() -> Integer

Thread safety: SAFE
Returns the thread constant identifying which thread the caller is executing on.
Both the real-time audio thread and offline audio export thread return Threads.Audio.

Dispatch/mechanics:
  KillStateHandler::getCurrentThread() -> maps TargetThread to LockHelpers::Type
    AudioThread/AudioExportThread -> AudioLock (4)
    MessageThread -> MessageLock (0)
    SampleLoadingThread -> SampleLock (2)
    ScriptingThread -> ScriptLock (1)

Pair with:
  toString -- convert the returned constant to a human-readable name

Source:
  ScriptingApi.cpp:7864  constructor registers constants mapping to LockHelpers::Type
  ScriptingApi.h:1860  getCurrentThread() const
    -> KillStateHandler::getCurrentThread() -> getAsLockId() cast to int
