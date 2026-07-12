BackgroundTask::setStatusMessage(String m) -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock::ScopedWriteLock and performs String assignment.
Sets the status message for this task. Protected by a read-write lock, pollable
from any thread via getStatusMessage().
Pair with:
  getStatusMessage -- reads the message this method sets
  setForwardStatusToLoadingThread -- enables forwarding to loading overlay
Source:
  ScriptingApiObjects.cpp  setStatusMessage()
    -> SimpleReadWriteLock::ScopedWriteLock -> message = m
    -> if forwardToLoadingThread: getSampleManager().setCurrentPreloadMessage(m)
