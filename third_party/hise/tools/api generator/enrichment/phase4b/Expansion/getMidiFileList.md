Expansion::getMidiFileList() -> Array

Thread safety: UNSAFE -- heap allocations for array and string construction
Returns pool reference strings for all MIDI files in this expansion.
Does not trigger filesystem discovery -- relies on pool populated during expansion initialisation.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Pair with:
  getWildcardReference -- build full pool reference from returned relative paths

Source:
  ScriptExpansion.cpp:1542  ScriptExpansionReference wrapper
    -> pool->getMidiFilePool().getListOfAllReferences(true)
