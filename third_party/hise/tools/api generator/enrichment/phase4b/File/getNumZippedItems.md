File::getNumZippedItems() -> Integer

Thread safety: UNSAFE -- reads ZIP file from disk to parse central directory (I/O).
Returns the number of entries in this ZIP archive. Returns 0 if the file is not
a valid ZIP or does not exist.

Anti-patterns:
  - Creates a new ZipFile parser on each call (not cached). Store the result in a
    variable rather than calling this method in a loop.

Pair with:
  extractZipFile -- to extract the archive after checking entry count

Source:
  ScriptingApiObjects.cpp  ScriptFile::getNumZippedItems()
    -> juce::ZipFile zipFile(f); return zipFile.getNumEntries();
