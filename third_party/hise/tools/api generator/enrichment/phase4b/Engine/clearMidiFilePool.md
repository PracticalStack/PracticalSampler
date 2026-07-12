Engine::clearMidiFilePool() -> undefined

Thread safety: UNSAFE -- clears pool data structures (heap deallocation, string operations)
Removes all cached entries from the MIDI file pool. Backend-only (HISE IDE) -- compiled
to a no-op in exported plugins. Prints the number of removed entries to console.
Anti-patterns:
  - Do NOT rely on this in compiled plugins -- it is a complete no-op (USE_BACKEND guard)
Pair with:
  clearSampleMapPool -- clears the sample map pool
  rebuildCachedPools -- clears and reloads both pools
Source:
  ScriptingApi.cpp  Engine::clearMidiFilePool()
    -> pool->clearData() [USE_BACKEND only]
