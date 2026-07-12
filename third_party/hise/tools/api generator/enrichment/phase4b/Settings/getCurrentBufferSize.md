Settings::getCurrentBufferSize() -> Integer

Thread safety: UNSAFE -- accesses device manager to query current block size
Returns the current audio buffer size in samples.
Returns 0 if no audio device manager is available.

Anti-patterns:
  - Do NOT assume 0 means "no buffering" -- 0 indicates no device is available.
    getCurrentSampleRate returns -1 for the same condition.

Pair with:
  getAvailableBufferSizes -- list valid buffer sizes
  setBufferSize -- apply a new buffer size

Source:
  ScriptingApi.cpp  Settings::getCurrentBufferSize()
    -> driver->getCurrentBlockSize()
