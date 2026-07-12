Sampler::selectSounds(String regexWildcard) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Populates the legacy script selection by matching sample file names against a
regex pattern. Consider using the modern createSelection() API instead.
Pair with:
  getNumSelectedSounds -- get selection count
  getSoundProperty / setSoundProperty -- read/write properties in the selection
  createSelection -- preferred modern alternative
Source:
  ScriptingApi.cpp  Sampler::selectSounds()
    -> ModulatorSamplerSound::selectSoundsBasedOnRegex -> fills soundSelection
