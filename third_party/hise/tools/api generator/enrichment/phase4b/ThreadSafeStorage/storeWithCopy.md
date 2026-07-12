ThreadSafeStorage::storeWithCopy(var dataToStore) -> undefined

Thread safety: UNSAFE -- allocates a deep copy via clone(), then acquires exclusive write lock via store()
Stores a deep copy of the value, breaking reference semantics. The caller can safely
modify the original after this call without affecting stored data. For primitives
(numbers, booleans), behaves identically to store().

Required setup:
  const var tss = Engine.createThreadSafeStorage();

Dispatch/mechanics:
  dataToStore.clone() -> store(copy)
  String path has a bug: calls copy.toString() instead of dataToStore.toString()

Pair with:
  store -- use when reference sharing is acceptable (no post-store mutation)
  load/tryLoad -- to read back the stored value

Anti-patterns:
  - Do NOT use for string values -- stores an empty string due to a bug where
    copy.toString() is called on a default-constructed var instead of the input.
    Use store() for strings as a workaround.

Source:
  ScriptingApiObjects.cpp:8243  ScriptThreadSafeStorage::storeWithCopy()
    -> var::clone() for arrays/objects
    -> store(copy)
    -> BUG line 8248: copy.toString() should be dataToStore.toString()
