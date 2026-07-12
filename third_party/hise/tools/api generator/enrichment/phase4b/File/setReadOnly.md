File::setReadOnly(Integer shouldBeReadOnly, Integer applyRecursively) -> undefined

Thread safety: UNSAFE -- modifies filesystem permissions (OS I/O call). May recurse into subdirectories.
Sets or clears the read-only attribute on this file or directory. When
applyRecursively is true and this is a directory, applies to all children.

Pair with:
  hasWriteAccess -- to verify the permission change took effect

Anti-patterns:
  - No return value to indicate success or failure. Use hasWriteAccess() after
    calling to verify.
  - When applyRecursively is true, modifies all files and subdirectories with no
    confirmation or undo.

Source:
  ScriptingApiObjects.cpp  ScriptFile::setReadOnly()
    -> f.setReadOnly(shouldBeReadOnly, applyRecursively)
