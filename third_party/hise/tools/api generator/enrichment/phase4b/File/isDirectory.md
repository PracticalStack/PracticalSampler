File::isDirectory() -> Integer

Thread safety: UNSAFE -- queries filesystem to check directory status (OS stat call).
Returns true if this path exists on disk and is a directory, false otherwise.
Returns false for non-existent paths without error.

Source:
  ScriptingApiObjects.cpp  ScriptFile::isDirectory()
    -> f.isDirectory()
