Returns the currently stored value. Multiple concurrent readers are allowed, but the call blocks if a write is in progress. Returns undefined if nothing has been stored or after `clear()`.

> [!Warning:Do not call from the audio thread] `load()` blocks the calling thread while a writer holds the lock. Use `tryLoad()` for any read path that might run on the audio thread.