File::isSameFileAs(ScriptObject otherFile) -> Integer

Thread safety: SAFE -- no I/O, compares in-memory path strings only.
Returns true if this file and the given File object reference the same path.
Comparison is case-sensitive on Linux, case-insensitive on Windows/macOS.

Anti-patterns:
  - Passing a non-File object (e.g., a string path) silently returns false
    instead of reporting an error.
  - Does not resolve symlinks or HISE link file redirects. Two File objects
    pointing to the same physical file via different paths compare as different.

Source:
  ScriptingApiObjects.cpp  ScriptFile::isSameFileAs()
    -> dynamic_cast<ScriptFile*>(otherFile.getObject())
    -> sf->f == f  (JUCE File operator==)
