FixObjectFactory::setCompareFunction(NotUndefined newCompareFunction) -> undefined

Thread safety: UNSAFE -- constructs string objects internally, iterates container list to propagate. When set to a JavaScript function, every subsequent comparison invokes a synchronous script callback.
Sets comparison function for all arrays and stacks created by this factory.
Used by sort, indexOf, and contains operations. Accepts three input modes:
(1) Single property name string -- optimized C++ comparator, no script overhead.
(2) Comma-separated 2-4 property names -- sequential priority comparison.
(3) JavaScript function(a, b) returning -1/0/1 -- full flexibility, script overhead.
Passing any other value resets to default (byte equality + pointer ordering).
Propagates retroactively to ALL previously created arrays and stacks.

Callback signature: f(FixObject a, FixObject b)

Dispatch/mechanics:
  String without comma -> looks up property in layout -> creates NumberComparator<T>
    templated on DataType (int/float/bool) with direct memory read at offset
  String with comma -> tokenizes -> creates MultiComparator<N> (N=2..4)
    comparing properties in priority order
  Function -> stores as WeakCallbackHolder(2 args), called via callSync()
  Other -> resets to BIND_MEMBER_FUNCTION_2(Factory::compare)
  Then iterates arrays[] to propagate compareFunction to all containers

Pair with:
  createArray/createStack -- comparator applies to all containers from this factory
  FixObjectArray.sort/indexOf/contains -- operations that use the comparator
  FixObjectStack.insert/removeElement -- uses comparator for duplicate/lookup

Anti-patterns:
  - Do NOT use a JavaScript comparison function in performance-critical code --
    callSync() on every comparison is non-realtime-safe. Use the string-based
    property comparator instead.
  - Do NOT expect per-container comparators -- the factory propagates to ALL
    containers. There is no way to set independent comparators per container.
  - Do NOT pass more than 4 comma-separated properties -- throws a script error.
    Use a custom function for 5+ properties.

Source:
  FixLayoutObjects.cpp  Factory::setCompareFunction()
    -> String mode: creates NumberComparator<T, IsArray> or MultiComparator<N>
    -> Function mode: WeakCallbackHolder(getScriptProcessor(), this, func, 2)
    -> propagates via: for(auto& a: arrays) a->compareFunction = compareFunction
  FixLayoutObjects.cpp  Factory::compare()
    -> customCompareFunction.callSync(args, 2, &result) or byte-level fallback
