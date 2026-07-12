Engine::loadAudioFileIntoBufferArray(String audioFileReference) -> Array

Thread safety: UNSAFE -- file I/O, heap allocations (VariantBuffer creation)
Loads an audio file and returns array of Buffer objects (one per channel).
Supports {PROJECT_FOLDER} and expansion wildcard references. Uses pool caching.
Anti-patterns:
  - Returned Buffers wrap pool memory directly -- modifying buffer data modifies the
    cached pool entry affecting all subsequent accesses
Source:
  ScriptingApi.cpp  Engine::loadAudioFileIntoBufferArray()
    -> PoolReference -> pool->loadFromReference(LoadAndCacheStrong)
    -> wraps channel data in VariantBuffer objects
