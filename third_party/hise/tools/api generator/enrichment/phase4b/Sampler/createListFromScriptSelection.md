Sampler::createListFromScriptSelection() -> Array

Thread safety: UNSAFE -- allocates ScriptingSamplerSound objects on the heap
Converts the legacy script selection (from selectSounds()) into an array of
Sample objects. Bridges the legacy selection API to the modern Sample-based API.
Pair with:
  selectSounds -- populates the legacy selection that this method reads
  createSelection -- preferred modern alternative (no legacy selection step needed)
Source:
  ScriptingApi.cpp  Sampler::createListFromScriptSelection()
    -> iterates soundSelection items
    -> creates ScriptingSamplerSound for each
