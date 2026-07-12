Engine::clearSampleMapPool() -> undefined

Thread safety: UNSAFE -- clears pool data structures (heap deallocation, string operations)
Removes all cached entries from the sample map pool. Backend-only (HISE IDE) -- compiled
to a no-op in exported plugins. Prints the number of removed entries to console.
Anti-patterns:
  - Do NOT rely on this in compiled plugins -- it is a complete no-op (USE_BACKEND guard)
Pair with:
  clearMidiFilePool -- clears the MIDI file pool
  rebuildCachedPools -- clears and reloads both pools
Source:
  ScriptingApi.cpp  Engine::clearSampleMapPool()
    -> pool->clearData() [USE_BACKEND only]
