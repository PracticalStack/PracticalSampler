File::getNonExistentSibling() -> ScriptObject

Thread safety: UNSAFE -- queries filesystem to check file existence (I/O).
Returns a File object for a sibling path that does not currently exist on disk.
If this file does not exist, returns a File for the same path. If it does exist,
appends a numeric suffix (e.g., " (2)") before the extension to find a unique name.

Anti-patterns:
  - The returned path is only guaranteed unique at the moment of the call. A
    concurrent process could create a file at that path before you write to it.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getNonExistentSibling()
    -> new ScriptFile(getScriptProcessor(), f.getNonexistentSibling(false))
