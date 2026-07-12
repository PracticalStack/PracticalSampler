BackgroundTask::setProperty(String id, var value) -> undefined

Thread safety: UNSAFE -- acquires SimpleReadWriteLock::ScopedWriteLock to write to the synchronized property store. Also constructs a juce::Identifier from the string key.
Stores a key-value pair in the thread-safe property store. The background
thread can publish intermediate results for UI polling via getProperty().
Pair with:
  getProperty -- reads values stored by this method
Source:
  ScriptingApiObjects.cpp  setProperty()
    -> Identifier(id) -> SimpleReadWriteLock::ScopedWriteLock
    -> synchronisedData.set(id, value)
