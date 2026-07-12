Sampler::clearSampleMap() -> Integer

Thread safety: UNSAFE -- kills all voices via killAllVoicesAndCall, lambda allocation
Clears the current sample map by removing all samples. Returns true on success.
Anti-patterns:
  - Do NOT treat the return value as synchronous confirmation -- the clear is
    scheduled via killAllVoicesAndCall and may not be complete when the method returns
Pair with:
  loadSampleMap -- to load a new map after clearing
  loadSampleMapFromJSON -- to rebuild from JSON after clearing
Source:
  ScriptingApi.cpp  Sampler::clearSampleMap()
    -> s->killAllVoicesAndCall(f) -> sm->clear(sendNotificationAsync)
