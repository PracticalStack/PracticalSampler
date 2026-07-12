FixObjectFactory::createStack(int numElements) -> ScriptObject

Thread safety: UNSAFE -- allocates a contiguous heap block for all elements
Creates a fixed-capacity stack of FixObjects with insert/remove semantics and a
position pointer tracking active element count. Capacity is pre-allocated but only
elements up to the current position are active. Uses swap-and-pop for removal and
duplicate checking via indexOf on insertion. Inherits the factory's current compare
function (live, not snapshot). Factory retains a reference.
Returns undefined if the factory's layout description was invalid.

Required setup:
  const var f = Engine.createFixObjectFactory({"id": 0, "value": 0.0});

Dispatch/mechanics:
  Factory::createStack(n) -> new fixobj::Stack(processor, n)
    -> stack.compareFunction = factory.compareFunction
    -> stack.init(factory)  // copies layout, allocates contiguous block
    -> arrays.add(stack)    // stored in same array list as createArray results

Pair with:
  create -- make a reusable temp object for populating before insert()
  setCompareFunction -- configure comparator for indexOf/contains/insert
    duplicate checking and removeElement lookups

Anti-patterns:
  - Do NOT assume failure throws an error -- silently returns undefined for
    invalid factory prototypes.

Source:
  FixLayoutObjects.cpp  Factory::createStack()
    -> new Stack(getScriptProcessor(), numElements)
    -> compareFunction assignment + init(this)
    -> arrays.add(newElement)
