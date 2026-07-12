Threads::isLockedByCurrentThread(Integer thread) -> Integer

Thread safety: SAFE
Returns true if the calling thread currently holds the lock identified by the
given thread constant. Use to verify lock ownership before performing operations
that require a specific lock.

Dispatch/mechanics:
  KillStateHandler::currentThreadHoldsLock(LockHelpers::Type)

Pair with:
  isLocked -- checks if any thread holds the lock (not caller-specific)
  getLockerThread -- returns which lock a given thread holds

Source:
  ScriptingApi.h:1860  isLockedByCurrentThread() const
    -> KillStateHandler::currentThreadHoldsLock(getAsLockId(thread))
