Sampler::loadSfzFile(var sfzFile) -> var

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, file I/O, parsing, killAllVoicesAndCall
Loads a sample map from an SFZ file. Accepts a ScriptFile object or an absolute
path string. Returns undefined on success, or an error string on failure.
Anti-patterns:
  - Do NOT check for success with `if (result)` -- uses inverted return convention:
    undefined means success, a String means failure. Use isDefined(result) to detect errors.
Pair with:
  loadSampleMap -- alternative: load from HISE sample map format
  loadSampleMapFromJSON -- alternative: load from JSON array
Source:
  ScriptingApi.cpp  Sampler::loadSfzFile()
    -> SfzImporter -> s->killAllVoicesAndCall(...)
