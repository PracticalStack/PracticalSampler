Engine::loadImageIntoPool(String id) -> undefined

Thread safety: UNSAFE -- file I/O, image decoding. Backend only -- no-op in compiled plugins.
Loads image(s) into the image pool. Supports wildcard patterns with *.
Anti-patterns:
  - No-op in compiled plugins -- images must be referenced by UI components for embedding
  - Wildcard is a simple contains() check, not true glob
Source:
  ScriptingApi.cpp  Engine::loadImageIntoPool()
    -> [backend] pool->loadFromReference(LoadAndCacheStrong)
    -> [frontend] no-op
