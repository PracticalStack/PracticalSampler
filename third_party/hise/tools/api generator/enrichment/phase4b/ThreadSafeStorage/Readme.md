ThreadSafeStorage (object)
Obtain via: Engine.createThreadSafeStorage()

Lock-based container for safely passing arbitrary data between audio and scripting
threads. Uses a read-write lock backed by an audio-optimized spin mutex to allow
concurrent readers while serializing writes.

Complexity tiers:
  1. Simple value passing: store, load. Passing primitive values or small objects
     between callbacks on the same thread type.
  2. Audio-thread consumption: + tryLoad. Non-blocking reads from the audio callback
     with a valid fallback value.
  3. Mutable source data: + storeWithCopy. Writer continues to modify the original
     data after storing; deep copy breaks reference sharing.

Practical defaults:
  - Use store() by default. Only use storeWithCopy() when the caller will continue
    to mutate the stored object after the call.
  - Use tryLoad() for any read that might run on the audio thread. Use load() only
    when certain the reader is on the UI/scripting thread.
  - Choose the tryLoad() fallback to be a valid operational default (e.g., 0.0 for
    gain, [] for a data list), not a sentinel like -1 that requires special handling.

Common mistakes:
  - Using load() on the audio thread -- blocks if a write is in progress. Use
    tryLoad() instead.
  - Calling store() then mutating the original array/object -- store() shares the
    reference, creating a race condition. Use storeWithCopy() if you keep modifying
    the source.
  - Passing storeWithCopy() a string value -- returns empty string due to a bug in
    the implementation. Use store() for strings as a workaround.
  - Reading ThreadSafeStorage inside a LAF paint callback via load() -- use it as a
    write-only mailbox from paint routines, read from a Broadcaster listener or timer.

Example:
  // Create a thread-safe storage for passing data to the audio thread
  const var tss = Engine.createThreadSafeStorage();

  // Store data from the UI thread
  tss.store({ "gain": 0.5, "pan": -0.2 });

  // Read from audio thread (non-blocking)
  const var data = tss.tryLoad({ "gain": 1.0, "pan": 0.0 });

Methods (5):
  clear       load        store
  storeWithCopy           tryLoad
