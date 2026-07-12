File::writeObject(JSON jsonData) -> Integer

Thread safety: UNSAFE -- performs filesystem I/O (JSON serialization and file write).
Serializes a JSON value to formatted text and writes to this file.
Uses JUCE default formatting (indented, human-readable). Returns true on success.

Dispatch/mechanics:
  JSON::toString(jsonData) -> writeString(text)

Pair with:
  loadAsObject -- to read the JSON back
  writeEncryptedObject -- for encrypted persistence
  writeAsXmlFile -- for XML output

Source:
  ScriptingApiObjects.cpp  ScriptFile::writeObject()
    -> JSON::toString(jsonData) -> writeString(text)
