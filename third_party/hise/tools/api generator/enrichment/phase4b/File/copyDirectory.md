File::copyDirectory(ScriptObject target) -> Integer

Thread safety: UNSAFE -- performs recursive filesystem I/O (directory copy).
Recursively copies this directory to the target location. The target is the
actual directory to create, not the parent to copy into. Returns true on success.

Required setup:
  const var sourceDir = FileSystem.getFolder(FileSystem.Documents).getChildFile("myFolder");
  const var targetDir = FileSystem.getFolder(FileSystem.Documents).getChildFile("myFolder_copy");

Dispatch/mechanics:
  dynamic_cast<ScriptFile*>(target) -> checks sf->f.isDirectory()
  -> f.copyDirectoryTo(sf->f)
  Reports script error if target is not a File object or not a directory.

Pair with:
  copy -- for single file copying
  move -- to relocate instead of duplicate

Anti-patterns:
  - [BUG] If target exists as a file (not directory), the error "target is not a
    directory" is reported but execution continues to copyDirectoryTo() anyway due
    to a missing early return.
  - Do NOT pass the parent directory as target -- the target IS the destination
    directory path itself.

Source:
  ScriptingApiObjects.cpp  ScriptFile::copyDirectory()
    -> dynamic_cast<ScriptFile*>(target.getObject())
    -> sf->f.isDirectory() check (bug: no early return on failure)
    -> f.copyDirectoryTo(sf->f)
