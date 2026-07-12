ThreadSafeStorage::load() -> NotUndefined

Thread safety: UNSAFE -- acquires blocking shared read lock via ScopedReadLock. Use tryLoad() for audio-thread reads.
Returns the currently stored value. Blocks if a writer is active. Multiple concurrent readers allowed.
Returns undefined if nothing has been stored or after clear().

Required setup:
  const var tss = Engine.createThreadSafeStorage();
  tss.store(someData);

Pair with:
  tryLoad -- non-blocking alternative for audio thread
  store -- to write data that load() reads

Anti-patterns:
  - Do NOT call from the audio thread -- blocks if a write is in progress. Use tryLoad(fallback) instead.

Source:
  ScriptingApiObjects.cpp:8255  ScriptThreadSafeStorage::load()
    -> ScopedReadLock(lock) -> return data
