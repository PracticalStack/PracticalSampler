BackgroundTask::getProperty(String id) -> var

Thread safety: UNSAFE -- acquires SimpleReadWriteLock::ScopedReadLock to read from the synchronized property store.
Returns the value stored under the given key. Returns undefined if the key has
not been set. Concurrent reads are safe while the background thread writes.
Pair with:
  setProperty -- stores values that this method reads
Source:
  ScriptingApiObjects.cpp  getProperty()
    -> Identifier(id) -> SimpleReadWriteLock::ScopedReadLock
    -> synchronisedData.getWithDefault(id, var())
