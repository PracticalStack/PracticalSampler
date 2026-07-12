File::getSize() -> Integer

Thread safety: UNSAFE -- queries filesystem metadata (OS I/O call).
Returns the file size in bytes. Returns 0 if the file does not exist or is a
directory.

Source:
  ScriptingApiObjects.cpp  ScriptFile::getSize()
    -> f.getSize()
