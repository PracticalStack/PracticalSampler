AudioFile::getCurrentlyLoadedFile() -> String

Thread safety: WARNING -- String return, atomic ref-count operations.
Returns the HISE pool reference string of the currently loaded audio file
(e.g. "{PROJECT_FOLDER}audiofile.wav"). Returns empty string if no file is loaded.

Anti-patterns:
  - Do NOT treat the return value as a filesystem path -- it is a HISE pool
    reference string. Format depends on the data provider.

Source:
  ScriptingApiObjects.cpp  getCurrentlyLoadedFile()
    -> buffer->toBase64String() (returns reference string, not actual base64)
