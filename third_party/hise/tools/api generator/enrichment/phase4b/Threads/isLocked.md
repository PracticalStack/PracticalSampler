Threads::isLocked(Integer thread) -> Integer

Thread safety: SAFE
Returns true if the specified thread currently holds any lock. Delegates to
getLockerThread and checks whether the result is anything other than Threads.Free.

Dispatch/mechanics:
  getLockerThread(thread) -> cast to LockId -> compare != LockId::unused

Pair with:
  getLockerThread -- returns which lock is held (not just boolean)
  isLockedByCurrentThread -- checks if the calling thread holds a specific lock

Source:
  ScriptingApi.cpp  isLocked()
    -> auto t = (LockId)getLockerThread(thread); return t != LockId::unused;
