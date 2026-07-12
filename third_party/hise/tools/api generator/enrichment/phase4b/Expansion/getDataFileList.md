Expansion::getDataFileList() -> Array

Thread safety: UNSAFE -- heap allocations for array and string construction
Returns pool reference strings for all data files in this expansion's AdditionalSourceCode pool.
Does not trigger filesystem discovery -- returns only files already known to the pool.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Pair with:
  loadDataFile -- load a specific data file by relative path
  writeDataFile -- write a JSON data file to the AdditionalSourceCode folder

Source:
  ScriptExpansion.cpp:1542  ScriptExpansionReference wrapper
    -> pool->getAdditionalDataPool().getListOfAllReferences(true)
