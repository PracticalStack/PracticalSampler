Sampler::setSoundProperty(Integer soundIndex, Integer propertyIndex, var newValue) -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Sets a sound property for a specific sound in the legacy script selection.
Use Sampler constants (e.g., Sampler.Volume, Sampler.Root) for propertyIndex.
Anti-patterns:
  - Do NOT confuse parameter order -- setSoundProperty takes (soundIndex, propertyIndex, value)
    but getSoundProperty takes (propertyIndex, soundIndex). This inconsistency causes
    hard-to-diagnose bugs.
Pair with:
  getSoundProperty -- read properties (note reversed parameter order)
  selectSounds -- populates the legacy selection
  setSoundPropertyForSelection -- apply to entire selection at once
Source:
  ScriptingApi.cpp  Sampler::setSoundProperty()
    -> soundSelection.getSelectedItem(soundIndex)
