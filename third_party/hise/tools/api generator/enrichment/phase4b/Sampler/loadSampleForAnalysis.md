Sampler::loadSampleForAnalysis(Integer soundIndex) -> Array

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, loads audio data into memory
Loads a sample from the legacy script selection into a buffer array for analysis.
Returns audio data as an array of buffers (one per channel). The soundIndex
refers to the selection populated by selectSounds().
Pair with:
  selectSounds -- populates the legacy selection
  getNumSelectedSounds -- get valid index range
  createSelection -- modern alternative for obtaining Sample objects
Source:
  ScriptingApi.cpp  Sampler::loadSampleForAnalysis()
    -> reads from soundSelection
