Expansion::getImageList() -> Array

Thread safety: UNSAFE -- calls loadAllFilesFromProjectFolder() (filesystem I/O), plus heap allocations for array and string construction
Returns pool reference strings for all image files in this expansion.
Forces filesystem discovery before listing -- first call may be slower than subsequent calls.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  loadAllFilesFromProjectFolder(ImagePool)
    -> pool->getImagePool().getListOfAllReferences(true)
    -> converts PoolReference list to Array<var> of strings

Pair with:
  getWildcardReference -- build full pool reference from returned relative paths

Source:
  ScriptExpansion.cpp:1542  ScriptExpansionReference wrapper
    -> pool->getImagePool().getListOfAllReferences(true)
    -> calls loadAllFilesFromProjectFolder() before listing (unlike getSampleMapList/getMidiFileList)
