File::writeString(String text) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (file write).
Writes the given text to this file, replacing existing content. Uses atomic
write via a temporary file to prevent data loss on failure. On Linux, forces
LF line endings. Returns true on success.

Dispatch/mechanics:
  #if JUCE_LINUX: f.replaceWithText(text, false, false, "\n")
  #else: f.replaceWithText(text)
  Other write methods (writeObject, writeAsXmlFile) delegate to this internally.

Pair with:
  loadAsString -- to read text back from disk
  writeObject -- higher-level JSON writing that delegates here

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeString()
    -> f.replaceWithText(text) [with Linux LF override]
