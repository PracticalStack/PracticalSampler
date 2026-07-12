File::loadAsBase64String() -> String

Thread safety: UNSAFE -- reads entire file from disk into memory (I/O).
Reads the file as raw binary data and returns a Base64-encoded string.
No compression is applied despite what the Doxygen comment claims.
Returns an empty string if the file does not exist or cannot be read.

Anti-patterns:
  - Base64 encoding increases data size by approximately 33%.
  - Reads entire file into memory before encoding. Not suitable for large files.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAsBase64String()
    -> f.loadFileAsData(mb) -> mb.toBase64Encoding()
