Sampler::createSelection(String regex) -> Array

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, allocates ScriptingSamplerSound objects
Creates an array of Sample objects matching the regex pattern against sample file
names. Modern replacement for the legacy selectSounds() workflow.
Required setup:
  const var sampler = Sampler;
  const var kicks = sampler.createSelection(".*kick.*");
Pair with:
  createSelectionFromIndexes -- select by index instead of regex
  createSelectionWithFilter -- select by callback filter function
Source:
  ScriptingApi.cpp  Sampler::createSelection()
    -> iterates all sounds, matches filename against regex
    -> creates ScriptingSamplerSound for each match
