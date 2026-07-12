File::deleteFileOrDirectory() -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (deletion).
Deletes the file or directory at this path WITHOUT confirmation. For directories,
deletion is recursive -- all contents are removed. Symlinks are not followed.
Returns true on success, false if not found or deletion fails.

Dispatch/mechanics:
  if (!f.isDirectory() && !f.existsAsFile()) return false;
  return f.deleteRecursively(false);
  The false parameter means symlinks are not followed during recursive deletion.

Pair with:
  isFile/isDirectory -- to check existence before deletion
  createDirectory -- to recreate after deletion

Anti-patterns:
  - Do NOT use the File object after deletion expecting it to be invalid -- the
    internal path is immutable. Calling isFile() or loadAsString() on the deleted
    path returns false/empty but does not error.
  - Recursive directory deletion has no confirmation prompt and no undo mechanism.

Source:
  ScriptingApiObjects.cpp  ScriptFile::deleteFileOrDirectory()
    -> f.deleteRecursively(false)
