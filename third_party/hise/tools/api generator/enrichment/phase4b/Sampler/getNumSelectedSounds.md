Sampler::getNumSelectedSounds() -> Integer

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Returns the number of sounds in the legacy script selection (populated by
selectSounds()). Part of the legacy selection API.
Pair with:
  selectSounds -- populates the legacy selection
  getSoundProperty -- reads properties from the selection
  createListFromScriptSelection -- converts legacy selection to modern Sample objects
Source:
  ScriptingApi.cpp  Sampler::getNumSelectedSounds()
    -> soundSelection.getNumSelected()
