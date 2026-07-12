BackgroundTask::getStatusMessage() -> String

Thread safety: UNSAFE -- acquires SimpleReadWriteLock::ScopedReadLock and returns a String.
Returns the current status message set by setStatusMessage(). Protected by a
read-write lock, allowing the UI thread to safely poll while the background
thread updates it.
Pair with:
  setStatusMessage -- sets the message this method reads
Source:
  ScriptingApiObjects.cpp  getStatusMessage()
    -> SimpleReadWriteLock::ScopedReadLock -> returns message String
