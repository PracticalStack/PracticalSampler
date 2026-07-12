FixObjectFactory (object)
Obtain via: Engine.createFixObjectFactory(layoutDescription)

Defines a typed memory schema from a JSON prototype and creates fixed-layout
containers (FixObjectArray, FixObjectStack) and individual objects (FixObject)
conforming to that schema. All containers share the factory's memory allocator
and layout definition. Supports int, float, bool, and fixed-size arrays of these.

Constants:
  Layout:
    prototype = (constructor arg)    The original JSON layout description object

Complexity tiers:
  1. Basic container: createArray, createStack. Fixed-size typed containers with
     direct property access in loops. No compare function needed.
  2. Searchable container: + setCompareFunction (string mode). Optimized
     indexOf/contains lookups and sorting by property name.
  3. Temp object + stack workflow: + create. Reusable template object populated
     per-event and inserted into a stack. Pair with a custom compare function
     for identity-based removal. Avoids per-event allocation.

Practical defaults:
  - Use createStack() when elements are added and removed dynamically (note
    tracking, particle pools). Use createArray() when element count is fixed.
  - A capacity of 128 is a good default for note-tracking stacks -- matches
    MIDI note range and typical maximum voice count.
  - Prefer the single-property string comparator (setCompareFunction("id"))
    over a custom function when sorting or searching by one field. Runs
    entirely in C++ with no script callback overhead.

Common mistakes:
  - Using string or object values in the prototype -- silently produces an
    invalid factory. Only int, float, bool, and arrays of these are supported.
  - Calling create() for every note-on event -- allocates heap memory each
    time. Create one temp object at init, rewrite its properties before each
    insert().
  - Using createArray() for a dynamically-sized collection -- FixObjectArray
    has no insert/remove. Use createStack() instead.
  - Passing more than 4 comma-separated properties to setCompareFunction --
    throws a script error. Use a custom function for 5+ properties.

Example:
  // Create a factory with a typed layout
  const var f = Engine.createFixObjectFactory({
      "id": 0,
      "velocity": 0.0,
      "active": false
  });

  // Create containers from the factory
  const var list = f.createArray(128);
  const var stack = f.createStack(16);

  // Set an optimized comparator by property name
  f.setCompareFunction("id");

Methods (5):
  create              createArray
  createStack         getTypeHash
  setCompareFunction
