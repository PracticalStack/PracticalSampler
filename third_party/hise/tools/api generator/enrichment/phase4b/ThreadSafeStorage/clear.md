ThreadSafeStorage::clear() -> undefined

Thread safety: UNSAFE -- acquires exclusive write lock via ScopedMultiWriteLock (delegates to store())
Resets the storage to an empty (undefined) state.

Dispatch/mechanics:
  clear() -> store(var()) -> ScopedMultiWriteLock + std::swap

Pair with:
  store -- to set a new value after clearing

Source:
  ScriptingApiObjects.cpp:8232  ScriptThreadSafeStorage::clear()
    -> store(var())
