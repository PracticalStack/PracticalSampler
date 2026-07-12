Expansion::loadDataFile(var relativePath) -> var

Thread safety: UNSAFE -- file I/O (FileBased) or pool loading (Intermediate/Encrypted), plus JSON parsing and heap allocation
Loads a JSON data file from this expansion's AdditionalSourceCode directory.
Behavior differs by expansion type: FileBased reads from filesystem, Intermediate/Encrypted
loads from embedded data pool with strong caching. Returns parsed JSON object.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  FileBased path:
    getSubDirectory(AdditionalSourceCode).getChildFile(relativePath)
      -> File::loadFileAsString() -> JSON::parse()
  Intermediate/Encrypted path:
    builds wildcard PoolReference -> pool->getAdditionalDataPool().loadFromReference(ref, LoadAndCacheStrong)
      -> JSON::parse(poolData)

Pair with:
  writeDataFile -- write JSON data to the same folder
  getDataFileList -- enumerate available data files

Anti-patterns:
  - Do NOT assume a missing file throws an error -- FileBased path silently returns
    undefined for missing files. Check with isDefined().
  - Do NOT expect writeDataFile changes to be visible via loadDataFile on
    Intermediate/Encrypted expansions -- writeDataFile writes to filesystem but
    loadDataFile reads from the embedded pool (stale data).

Source:
  ScriptExpansion.cpp:1753  ScriptExpansionReference::loadDataFile()
    -> branches on exp->getExpansionType() == Expansion::FileBased
    -> FileBased: filesystem read + JSON::parse()
    -> Other: PoolReference + loadFromReference(LoadAndCacheStrong) + JSON::parse()
