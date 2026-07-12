FixObjectFactory::create() -> ScriptObject

Thread safety: UNSAFE -- allocates heap memory via internal block allocator (HeapBlock construction)
Creates a single FixObject with the factory's layout. Members are initialized to
prototype default values. Factory retains a reference to the created object.
Returns undefined if the factory's layout description was invalid.

Required setup:
  const var f = Engine.createFixObjectFactory({"id": 0, "value": 0.0});

Dispatch/mechanics:
  Factory::create() -> allocator->allocate(elementSize)
    -> new ObjectReference() -> init(factory, block, resetToDefault=true)
    -> singleObjects.add(ref)  // factory retains ownership

Anti-patterns:
  - Do NOT call create() in a note-on handler or timer callback -- allocates
    heap memory every time. Create one temp object at init, rewrite properties
    before each stack insert().
  - Do NOT assume failure throws an error -- if the factory was constructed with
    an invalid prototype (strings, objects), create() silently returns undefined.

Source:
  FixLayoutObjects.cpp:304  Factory::create()
    -> allocator->allocate(getElementSizeInBytes())
    -> new ObjectReference() -> init(this, block, true)
    -> singleObjects.add(ref)
