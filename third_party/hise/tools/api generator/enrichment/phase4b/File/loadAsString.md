File::loadAsString() -> String

Thread safety: UNSAFE -- reads file from disk (I/O).
Reads the entire file content as text and returns it. Returns an empty string if
the file does not exist or cannot be read -- no error is reported.

Source:
  ScriptingApiObjects.cpp  ScriptFile::loadAsString()
    -> f.loadFileAsString()
