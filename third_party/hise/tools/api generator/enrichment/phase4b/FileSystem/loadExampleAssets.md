FileSystem::loadExampleAssets() -> undefined

Thread safety: UNSAFE -- backend-only, accesses BackendProcessor, creates ExampleAssetManager, heap allocations
Initializes the HISE example asset manager, which provides dummy audio files, MIDI files,
and filmstrip images for code snippets and examples. Backend-only -- in compiled plugins
the entire method body is compiled out (#if USE_BACKEND) and the call is a no-op.

Dispatch/mechanics:
  dynamic_cast<BackendProcessor*>(getMainController()) -> getAssetManager()
  -> ExampleAssetManager lazily created -> am->initialise() populates dummy resources

Anti-patterns:
  - Do NOT rely on this in exported plugins -- no-op in frontend builds. No error or
    warning is produced.

Source:
  ScriptingApi.cpp:7394  FileSystem::loadExampleAssets()
    -> #if USE_BACKEND -> BackendProcessor::getAssetManager() -> initialise()
