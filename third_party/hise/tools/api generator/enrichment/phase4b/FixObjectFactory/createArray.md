FixObjectFactory::createArray(int numElements) -> ScriptObject

Thread safety: UNSAFE -- allocates a contiguous heap block for all elements
Creates a fixed-size array of FixObjects. All elements share the factory's layout
and are initialized to default values. Inherits the factory's current compare
function (live, not snapshot). Factory retains a reference to the created array.
Returns undefined if the factory's layout description was invalid.

Required setup:
  const var f = Engine.createFixObjectFactory({"id": 0, "value": 0.0});

Dispatch/mechanics:
  Factory::createArray(n) -> new fixobj::Array(processor, n)
    -> array.compareFunction = factory.compareFunction
    -> array.init(factory)  // copies layout, allocates contiguous block
    -> arrays.add(array)    // factory retains ownership

Pair with:
  setCompareFunction -- configure before or after; propagates to this array
  createStack -- use instead when elements are added/removed dynamically

Anti-patterns:
  - Do NOT use createArray() for dynamically-sized collections -- FixObjectArray
    has no insert/remove. Use createStack() for add/remove workflows.
  - Do NOT assume failure throws an error -- silently returns undefined for
    invalid factory prototypes.

Source:
  FixLayoutObjects.cpp  Factory::createArray()
    -> new Array(getScriptProcessor(), numElements)
    -> compareFunction assignment + init(this)
    -> arrays.add(newElement)
