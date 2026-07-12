Engine::createFixObjectFactory(var layoutDescription) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, Allocator creation, layout parsing
Creates a fixed-layout object factory from a JSON layout description. Produces
memory-efficient objects with typed fields (Integer, Boolean, Float) using contiguous
memory with known offsets rather than hash-map lookups. Field types are inferred from
values: 0/int -> Integer, 0.0/float -> Float, false/true -> Boolean.
Anti-patterns:
  - Do NOT pass a non-object (array, string, number) as layoutDescription -- the factory
    is created but all subsequent create()/createArray()/createStack() calls silently
    return undefined with no error
Source:
  ScriptingApi.cpp  Engine::createFixObjectFactory()
    -> new fixobj::Factory(layoutDescription) -> parses into MemoryLayoutItem objects
