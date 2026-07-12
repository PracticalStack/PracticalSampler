AudioSampleProcessor::getFilename() -> String

Thread safety: WARNING -- String return value involves atomic ref-count operations.
Returns the pool reference string of the currently loaded audio file (e.g.,
{PROJECT_FOLDER}my_loop.wav or {EXP::myExpansion}file.wav). Not a filesystem path.
Returns empty string if no file is loaded.
Dispatch/mechanics:
  ProcessorWithExternalData->getAudioFile(0)->toBase64String()
Pair with:
  setFile -- load a file using the same pool reference format
Anti-patterns:
  - Do NOT use the return value as a filesystem path -- it is a pool reference string.
    Convert via FileSystem.fromReferenceString() for file system operations.
Source:
  ScriptingApiObjects.cpp:4997+  getFilename() -> getAudioFile(0)->toBase64String()
