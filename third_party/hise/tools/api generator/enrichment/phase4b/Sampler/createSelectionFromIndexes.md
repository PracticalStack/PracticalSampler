Sampler::createSelectionFromIndexes(Array indexData) -> Array

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, allocates ScriptingSamplerSound objects
Creates an array of Sample objects from sample indices. Accepts an array of
integer indices, a single integer index, or -1 to select all samples.
Pair with:
  createSelection -- select by regex instead of index
  createSelectionWithFilter -- select by callback filter
Source:
  ScriptingApi.cpp  Sampler::createSelectionFromIndexes()
    -> handles int, array, or -1 (all samples)
    -> creates ScriptingSamplerSound for each valid index
