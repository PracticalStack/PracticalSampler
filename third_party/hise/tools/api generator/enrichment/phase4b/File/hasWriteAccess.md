File::hasWriteAccess() -> Integer

Thread safety: UNSAFE -- queries filesystem permissions (OS I/O call).
Returns true if the file or directory has write permissions for the current user.
Returns false if the file does not exist.

Pair with:
  setReadOnly -- to change write permissions

Source:
  ScriptingApiObjects.cpp  ScriptFile::hasWriteAccess()
    -> f.hasWriteAccess()
