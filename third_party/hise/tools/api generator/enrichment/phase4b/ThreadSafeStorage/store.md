ThreadSafeStorage::store(var dataToStore) -> undefined

Thread safety: UNSAFE -- acquires exclusive write lock via ScopedMultiWriteLock
Stores a value with reference semantics. For arrays and objects, the storage holds a
reference to the same underlying data -- mutations by the caller after storing also
affect the stored copy.

Required setup:
  const var tss = Engine.createThreadSafeStorage();

Dispatch/mechanics:
  ScopedMultiWriteLock(lock) -> std::swap(data, dataToStore)
  Old value destroyed OUTSIDE the lock (parameter destructor runs after lock release)

Pair with:
  storeWithCopy -- to break reference sharing when caller keeps modifying the source
  load/tryLoad -- to read back the stored value
  clear -- to reset to undefined

Anti-patterns:
  - Do NOT mutate the original array/object after store() -- the storage shares the
    reference, creating a race condition with readers on other threads. Use
    storeWithCopy() if you intend to keep modifying the data.

Source:
  ScriptingApiObjects.cpp:8237  ScriptThreadSafeStorage::store()
    -> ScopedMultiWriteLock(lock) -> std::swap(data, dataToStore)
