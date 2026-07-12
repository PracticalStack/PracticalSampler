Expansion::writeDataFile(var relativePath, var dataToWrite) -> bool

Thread safety: UNSAFE -- JSON serialization and file I/O (replaceWithText)
Writes a JSON data file to this expansion's AdditionalSourceCode directory on the
filesystem. Always writes to the filesystem regardless of expansion type -- does not
update embedded pool data. Returns true if the file was written successfully.

Required setup:
  const var e = Engine.createExpansionHandler().getExpansionList()[0];

Dispatch/mechanics:
  JSON::toString(dataToWrite)
    -> getSubDirectory(AdditionalSourceCode).getChildFile(relativePath)
    -> targetFile.replaceWithText(content)

Pair with:
  loadDataFile -- read the data file back (but see anti-patterns for non-FileBased)
  getDataFileList -- enumerate available data files

Anti-patterns:
  - [BUG] For Intermediate/Encrypted expansions, the written file exists on disk but
    does not modify the embedded pool. A subsequent loadDataFile() call on a non-FileBased
    expansion loads from the pool (old data), not the file just written.
  - [BUG] Does not check whether the expansion reference is still valid. If the
    expansion has been unloaded, this may crash.

Source:
  ScriptExpansion.cpp:1802  ScriptExpansionReference::writeDataFile()
    -> JSON::toString(dataToWrite)
    -> exp->getSubDirectory(AdditionalSourceCode).getChildFile(relativePath).replaceWithText(content)
