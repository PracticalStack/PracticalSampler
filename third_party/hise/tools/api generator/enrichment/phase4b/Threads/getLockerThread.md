Threads::getLockerThread(Integer threadThatIsLocked) -> Integer

Thread safety: SAFE
Returns the lock type currently held by the specified thread. Returns
Threads.Free if the thread does not hold any lock.

Dispatch/mechanics:
  getAsThreadId(threadThatIsLocked) -> KillStateHandler::getLockTypeForThread()
    -> returns LockHelpers::Type as int

Pair with:
  isLocked -- simplified boolean check (delegates to getLockerThread internally)
  isLockedByCurrentThread -- checks if the calling thread holds a specific lock

Source:
  ScriptingApi.cpp  getLockerThread()
    -> (int)getKillStateHandler().getLockTypeForThread(getAsThreadId(threadThatIsLocked))
