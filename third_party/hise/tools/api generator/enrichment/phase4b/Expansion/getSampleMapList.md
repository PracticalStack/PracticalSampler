Expansion::getSampleMapList() -> Array

Thread safety: UNSAFE -- heap allocations for array and string construction
Returns sample map reference strings for this expansion with .xml extension stripped.
Does not trigger filesystem discovery -- relies on pool populated during expansion initialisation.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Pair with:
  getWildcardReference -- build full pool reference from returned strings

Source:
  ScriptExpansion.cpp:1542  ScriptExpansionReference wrapper
    -> pool->getSampleMapPool().getListOfAllReferences(true)
    -> strips .xml extension from each reference string
