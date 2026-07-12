Sampler::getSoundProperty(Integer propertyIndex, Integer soundIndex) -> var

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD
Returns a sound property value from the legacy script selection. Use Sampler
constants (e.g., Sampler.Root, Sampler.LoKey) for propertyIndex.
Anti-patterns:
  - Do NOT confuse parameter order with setSoundProperty -- getSoundProperty takes
    (propertyIndex, soundIndex) but setSoundProperty takes (soundIndex, propertyIndex, value)
Pair with:
  setSoundProperty -- sets a property (note reversed parameter order)
  selectSounds -- populates the legacy selection
Source:
  ScriptingApi.cpp  Sampler::getSoundProperty()
    -> soundSelection.getSelectedItem(soundIndex)
