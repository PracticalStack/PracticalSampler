ExpansionHandler::getExpansion(var name) -> ScriptObject

Thread safety: UNSAFE -- allocates a ScriptExpansionReference wrapper on the heap.
Returns the expansion with the given name, or undefined if not found. Only searches
successfully initialized expansions.
Pair with:
  getExpansionList -- enumerate all available expansions
Source:
  ScriptExpansion.cpp  getExpansion()
    -> searches expansionList by Name property
    -> wraps match as ScriptExpansionReference
