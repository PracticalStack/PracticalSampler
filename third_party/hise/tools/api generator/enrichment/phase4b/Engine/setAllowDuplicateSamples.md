Engine::setAllowDuplicateSamples(bool shouldAllow) -> undefined

Thread safety: UNSAFE -- modifies sample pool flag (should be called during onInit)
Controls whether the sample pool deduplicates samples. false = shared buffers
(saves memory), true = independent copies (allows different processing).
Pair with:
  reloadAllSamples -- reload after changing this setting
Source:
  ScriptingApi.cpp  Engine::setAllowDuplicateSamples()
    -> ModulatorSamplerSoundPool::setAllowDuplicateSamples(shouldAllow)
