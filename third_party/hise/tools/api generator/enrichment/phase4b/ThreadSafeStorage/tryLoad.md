ThreadSafeStorage::tryLoad(var returnValueIfLocked) -> NotUndefined

Thread safety: SAFE -- non-blocking, uses try_lock_shared() which returns immediately if lock unavailable
Non-blocking read for audio-thread use. Returns stored value if lock acquired,
otherwise returns returnValueIfLocked. This is the recommended read path for
realtime callbacks.

Required setup:
  const var tss = Engine.createThreadSafeStorage();
  tss.store(someData);

Dispatch/mechanics:
  ScopedTryReadLock(lock) -> if acquired: return data, else: return returnValueIfLocked
  Also succeeds if calling thread is the current writer (safe reentrant read)

Pair with:
  store/storeWithCopy -- to write data that tryLoad() reads
  load -- blocking alternative for non-audio threads

Anti-patterns:
  - Do NOT use a sentinel fallback (e.g., -1) that requires special downstream
    handling. Use a valid operational default (e.g., 0.0 for gain, [] for data list).

Source:
  ScriptingApiObjects.cpp:8261  ScriptThreadSafeStorage::tryLoad()
    -> ScopedTryReadLock(lock) with operator bool()
    -> returns data or returnValueIfLocked
