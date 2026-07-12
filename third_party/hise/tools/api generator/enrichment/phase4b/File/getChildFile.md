File::getChildFile(String childFileName) -> ScriptObject

Thread safety: SAFE -- no I/O, constructs a path object only.
Returns a new File object for a child path relative to this directory.
The child does not need to exist on disk. Supports simple filenames and
relative paths with separators.

Pair with:
  getParentDirectory -- to navigate up
  createDirectory -- to create the child as a directory

Source:
  ScriptingApiObjects.cpp  ScriptFile::getChildFile()
    -> new ScriptFile(getScriptProcessor(), f.getChildFile(childFileName))
