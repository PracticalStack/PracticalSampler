Engine::createUnorderedStack() -> ScriptObject

Thread safety: UNSAFE -- heap allocation
Creates a fixed-capacity (128 floats) unordered collection optimized for audio-thread-safe
operations. Supports insert, remove, contains, clear without heap allocation after creation.
Does not preserve insertion order. Suitable for tracking active notes, voice IDs, or
numeric sets where order does not matter.
Source:
  ScriptingApi.cpp  Engine::createUnorderedStack()
    -> new ScriptUnorderedStack
