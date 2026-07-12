File::getParentDirectory() -> ScriptObject

Thread safety: SAFE -- no I/O, constructs a path object only.
Returns a new File object for the parent directory. If already at the filesystem
root, returns a File pointing to the root path.

Pair with:
  getChildFile -- to navigate down
  isChildOf -- to verify parent-child relationships

Source:
  ScriptingApiObjects.cpp  ScriptFile::getParentDirectory()
    -> new ScriptFile(getScriptProcessor(), f.getParentDirectory())
