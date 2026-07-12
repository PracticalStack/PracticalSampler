Engine::setPreloadMessage(String message) -> undefined

Thread safety: UNSAFE -- string construction, internal state change
Sets the preload message displayed during sample loading.
Pair with:
  getPreloadMessage -- read current message
  getPreloadProgress -- read numeric progress
Source:
  ScriptingApi.cpp  Engine::setPreloadMessage()
    -> SampleManager::setPreloadMessage(message)
