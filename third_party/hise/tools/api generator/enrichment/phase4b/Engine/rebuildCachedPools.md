Engine::rebuildCachedPools() -> undefined

Thread safety: UNSAFE -- file I/O, heap allocations
Clears and reloads MIDI file and sample map pools from project folder.
Backend-only -- complete no-op in compiled plugins.
Anti-patterns:
  - Complete no-op in compiled plugins (USE_BACKEND guard), no warning
Pair with:
  clearMidiFilePool/clearSampleMapPool -- clear individual pools
Source:
  ScriptingApi.cpp  Engine::rebuildCachedPools()
    -> pool->clearData() -> pool->loadAllFilesFromProjectFolder() [USE_BACKEND only]
