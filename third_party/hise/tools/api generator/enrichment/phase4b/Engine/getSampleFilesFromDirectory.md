Engine::getSampleFilesFromDirectory(String relativePathFromSampleFolder, Integer recursive) -> Array

Thread safety: UNSAFE -- filesystem I/O (directory listing)
Returns pool reference strings for .wav/.aif/.aiff files in a Samples subdirectory.
Backend-only -- returns empty array silently in compiled plugins.
Anti-patterns:
  - Do NOT rely on this in compiled plugins -- returns empty array with no warning
Source:
  ScriptingApi.cpp  Engine::getSampleFilesFromDirectory()
    -> File::findChildFiles() [USE_BACKEND only]
