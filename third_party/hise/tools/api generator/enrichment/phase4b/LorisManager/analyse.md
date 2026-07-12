LorisManager::analyse(ScriptObject file, Double rootFrequency) -> Integer

Thread safety: UNSAFE -- heavy FFT-based spectral analysis with heap allocations and DLL/library calls.
Analyses an audio file using the Loris partial tracking algorithm. Root frequency
(Hz) guides harmonic partial tracking. Returns true on success. Must be called
before all other Loris operations (synthesise, process, processCustom,
createEnvelopes, createEnvelopePaths, createSnapshot).
When caching is enabled (default), re-analysing the same file reuses cached partials.

Required setup:
  const var lm = Engine.getLorisManager();
  const var f = FileSystem.fromAbsolutePath("path/to/file.wav");

Dispatch/mechanics:
  initThreadController() -> LorisManager::analyse()
    -> loris_analyze(state, filePath, rootFrequency) via C API
    -> caches partial list if enablecache is true

Pair with:
  set -- configure analysis options (windowwidth, freqdrift, etc.) before calling
  synthesise/process/processCustom -- all require prior analyse

Anti-patterns:
  - Do NOT pass a file path string -- must be a File object from FileSystem.
    A non-File object silently returns false with no error.

Source:
  ScriptLorisManager.cpp  ScriptLorisManager::analyse()
    -> initThreadController()
    -> LorisManager::analyse() -> loris_analyze() via C API
