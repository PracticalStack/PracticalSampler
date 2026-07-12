File::createDirectory(String directoryName) -> ScriptObject

Thread safety: UNSAFE -- performs filesystem I/O (directory creation).
Creates a child directory with the given name inside this directory. If the
directory already exists, it is not recreated. Returns a File object pointing
to the child path regardless of whether creation succeeded.

Dispatch/mechanics:
  if (!f.getChildFile(directoryName).isDirectory())
    f.getChildFile(directoryName).createDirectory();
  return new ScriptFile(getScriptProcessor(), f.getChildFile(directoryName));

Pair with:
  getChildFile -- to construct paths without creating directories
  isDirectory -- to verify creation succeeded
  deleteFileOrDirectory -- to remove the created directory

Anti-patterns:
  - Do NOT assume creation succeeded just because a File object was returned.
    [BUG] The JUCE createDirectory() return value is not checked. Verify with
    isDirectory() on the returned File.

Source:
  ScriptingApiObjects.cpp  ScriptFile::createDirectory()
    -> f.getChildFile(directoryName).createDirectory()
    -> returns new ScriptFile wrapping the child path
