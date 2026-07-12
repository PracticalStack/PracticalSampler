Sampler::refreshInterface() -> undefined

Thread safety: UNSAFE -- WARN_IF_AUDIO_THREAD, sends async change messages
Sends change notifications to update the sampler interface and sound pool display.
Call after programmatically modifying sample properties to refresh the UI.
Pair with:
  setSoundPropertyForAllSamples -- bulk property modification
  setSoundPropertyForSelection -- selection property modification
Source:
  ScriptingApi.cpp  Sampler::refreshInterface()
    -> sends async change messages to UI
