File::isFile() -> Integer

Thread safety: UNSAFE -- queries filesystem to check file existence (OS stat call).
Returns true if this path exists on disk as a regular file (not a directory).
Returns false for non-existent paths and directories without error.

Source:
  ScriptingApiObjects.cpp  ScriptFile::isFile()
    -> f.existsAsFile()  // note: maps to existsAsFile(), not JUCE's isFile()
